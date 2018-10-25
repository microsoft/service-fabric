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
        class ParallelLogRecordsDispatcher
            : public LogRecordsDispatcher
        {
            K_FORCE_SHARED(ParallelLogRecordsDispatcher)

        public:

            static LogRecordsDispatcher::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in IOperationProcessor & processor,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in KAllocator & allocator);

        protected:

            ktl::Awaitable<bool> ProcessLoggedRecords() override;

        private:

            typedef KSharedArray<LogRecordLib::TransactionLogRecord::SPtr>::SPtr TransactionChainSPtr;

            ParallelLogRecordsDispatcher(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in IOperationProcessor & processor,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig);

            ktl::Awaitable<void> ProcessAtomicOperation(__in LogRecordLib::LogRecord & atomicOperation);

            void ProcessConcurrentTransactions(__in std::unordered_map<LONG64, TransactionChainSPtr> & concurrentTransactions);

            void ProcessConcurrentAtomicOperations(__in KArray<LogRecordLib::LogRecord::SPtr> & atomicOperations);

            //
            // Dispatches apply for a single transaction in order
            //
            ktl::Task ProcessSpawnedTransaction(__in TransactionChainSPtr & transaction);

            ktl::Task ProcessSpawnedAtomicOperation(__in LogRecordLib::LogRecord & atomicOperation);

            //
            // Invoked after processing a transaction or atomic operation
            //
            void ProcessedSpawnedTransaction();

            //
            // Dispatches apply for a single transaction in order
            //
            ktl::Awaitable<void> ProcessTransaction(__in TransactionChainSPtr & transaction);

            //
            // Determines the parallel apply logic amongst a set of unrelated transactions
            //
            ktl::Awaitable<LONG> SeparateTransactions();

            KArray<LogRecordLib::LogRecord::SPtr> concurrentRecords_;
            ULONG processingIndex_;
        };
    }
}
