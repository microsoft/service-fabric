// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class LogRecordsDispatcher
            : public KObject<LogRecordsDispatcher>
            , public KShared<LogRecordsDispatcher>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
            , public KThreadPool::WorkItem
        {
            K_FORCE_SHARED_WITH_INHERITANCE(LogRecordsDispatcher)

        public:

            virtual ktl::Awaitable<bool> ProcessLoggedRecords() = 0;

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

        protected:

            LogRecordsDispatcher(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in IOperationProcessor & processor,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig);

            void PrepareToProcessRecord(
                __in LogRecordLib::LogRecord & record,
                __in RecordProcessingMode::Enum processingMode);

            ktl::Awaitable<void> PauseDispatchingIfNeededAsync(LogRecordLib::LogRecord & record);

            void ScheduleStartProcessingLoggedRecords();

            bool IsProcessingCompleted();


            // Pointer to a configuration object shared throughout this replicator instance
            LONG64 numberOfSpawnedTransactions_;
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;
            KSharedArray<LoggedRecords::CSPtr>::SPtr processingRecords_;
            IOperationProcessor::SPtr recordsProcessor_;

        private:
            // Implementing the KThreadPool::WorkItem.
            void Execute();

            ktl::Task StartProcessingLoggedRecords();

            mutable KSpinLock dispatchLock_;
            KSharedArray<LoggedRecords::CSPtr>::SPtr flushedRecords_;

            mutable KSpinLock dispatchPauseAtBarrierLock_;
            LogRecordLib::LogRecord::SPtr lastDispatchingBarrier_;
            ktl::AwaitableCompletionSource<void>::SPtr dispatchPauseAtBarrierTcs_;
        };
    }
}
