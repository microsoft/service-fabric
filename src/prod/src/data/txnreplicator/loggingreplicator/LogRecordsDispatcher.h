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
        // Core logic of determining the order in which transactions are dispatched to the user
        // Also determines the parallel applies that can be dispatched to the user
        //
        class LogRecordsDispatcher final
            : public KObject<LogRecordsDispatcher>
            , public KShared<LogRecordsDispatcher>
            , public KThreadPool::WorkItem
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(LogRecordsDispatcher)

        public:

            static LogRecordsDispatcher::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in IOperationProcessor & processor,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in KAllocator & allocator);

            //
            // Invoked by the log writer after the log records have been written to disk
            //
            void DispatchLoggedRecords(__in LoggedRecords const & loggedRecords);

            // 
            // Invoked by loggingreplicatorimpl during change role to active secondary
            //
            ktl::Awaitable<NTSTATUS> DrainAndPauseDispatchAsync();

            // 
            // Invoked by loggingreplicatorimpl after change role to active secondary on state manager
            //
            void ContinueDispatch();

        private:

            typedef KSharedArray<LogRecordLib::TransactionLogRecord::SPtr>::SPtr TransactionChainSPtr;

            LogRecordsDispatcher(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in IOperationProcessor & processor,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig);

            void ScheduleStartProcessingLoggedRecords();

            // Implementing the KThreadPool::WorkItem.
            void Execute();

            bool IsProcessingCompleted();

            void PrepareToProcessRecord(
                __in LogRecordLib::LogRecord & record,
                __in RecordProcessingMode::Enum processingMode);

            ktl::Awaitable<void> ProcessAtomicOperations(__in KArray<LogRecordLib::LogRecord::SPtr> & atomicOperations);

            void ProcessConcurrentTransactions(__in std::unordered_map<LONG64, TransactionChainSPtr> & concurrentTransactions);

            //
            // Dispatches apply for a single transaction in order
            //
            ktl::Task ProcessSpawnedTransaction(__in TransactionChainSPtr & transaction);

            //
            // Invoked after processing a transaction
            //
            void ProcessedSpawnedTransaction();

            ktl::Awaitable<bool> ProcessLoggedRecords();

            //
            // Dispatches apply for a single atomic operation
            //
            ktl::Task ProcessSpawnedAtomicOperations(__in KArray<LogRecordLib::LogRecord::SPtr> & atomicOperations);

            //
            // Dispatches apply for a single transaction in order
            //
            ktl::Awaitable<void> ProcessTransaction(__in TransactionChainSPtr & transaction);
            
            //
            // Determines the parallel apply logic amongst a set of unrelated transactions
            //
            ktl::Awaitable<LONG> SeparateTransactions();

            ktl::Task StartProcessingLoggedRecords();

            ktl::Awaitable<void> PauseDispatchingIfNeededAsync(LogRecordLib::LogRecord & record);

            // Pointer to a configuration object shared throughout this replicator instance
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;
            bool const enableSecondaryCommitApplyAcknowledgement_;

            mutable KSpinLock dispatchLock_;
            KArray<LogRecordLib::LogRecord::SPtr> concurrentRecords_;
            KSharedArray<LoggedRecords::CSPtr>::SPtr flushedRecords_;
            LONG64 numberOfSpawnedTransactions_;
            ULONG processingIndex_;
            KSharedArray<LoggedRecords::CSPtr>::SPtr processingRecords_;
            IOperationProcessor::SPtr recordsProcessor_;

            mutable KSpinLock dispatchPauseAtBarrierLock_;
            LogRecordLib::LogRecord::SPtr lastDispatchingBarrier_;
            ktl::AwaitableCompletionSource<void>::SPtr dispatchPauseAtBarrierTcs_;
        };
    }
}
