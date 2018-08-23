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
        // Maintains state of pending/unstable transactions which are used to link new operations on existing transactions and also
        // to find oldest transaction during checkpoints
        //
        class TransactionMap final
            : public KObject<TransactionMap>
            , public KShared<TransactionMap>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(TransactionMap)

            friend class LoggingReplicatorTests::TransactionMapTest;

        public:

            static TransactionMap::SPtr Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in KAllocator & allocator);
            
            void Reuse();

            // These API's are used during regular functioning to link an operation to its parent transaction record and create and delete transaction state as they are being created and committed
            void AddOperation(__in LogRecordLib::OperationLogRecord & record);

            void CompleteTransaction(__in LogRecordLib::EndTransactionLogRecord & record);

            void CreateTransaction(__in LogRecordLib::BeginTransactionOperationLogRecord & record);

            // These API's are used during checkpoints
            LogRecordLib::BeginTransactionOperationLogRecord::SPtr GetEarliestPendingTransaction();

            LogRecordLib::BeginTransactionOperationLogRecord::SPtr GetEarliestPendingTransaction(
                __in LONG64 barrierLsn,
                __out bool & failedBarrierCheck);

            // These API's are used during false progress
            LogRecordLib::BeginTransactionOperationLogRecord::SPtr DeleteTransaction(__in LogRecordLib::BeginTransactionOperationLogRecord & record);

            LogRecordLib::OperationLogRecord::SPtr FindOperation(__in LogRecordLib::OperationLogRecord & record);

            LogRecordLib::BeginTransactionOperationLogRecord::SPtr FindTransaction(__in LogRecordLib::BeginTransactionOperationLogRecord & record);

            LogRecordLib::EndTransactionLogRecord::SPtr FindUnstableTransaction(__in LogRecordLib::EndTransactionLogRecord & record);

            LogRecordLib::OperationLogRecord::SPtr RedactOperation(
                __in LogRecordLib::OperationLogRecord & record,
                __in LogRecordLib::TransactionLogRecord & invalidTransactionLogRecord);

            // Reify the transaction - 
            // 1) Remove the begintx record associated with the EndTransactionLogRecord from the list of completed transactions. 
            // 2) Remove EndTransactionLogRecord from the list of unstable transactions. 
            // 3) The latest record associated with this transaction is updated to the EndTransactionLogRecord->ParentTransactionRecord
            // 4) Finally, the begin transaction record associated with EndTransactionLogRecord is marked as 'pending' by updating the 
            //    hashtable of pending transactions and inserted into the AVL tree representing LSN's associated with pending transactions. 

            LogRecordLib::EndTransactionLogRecord::SPtr ReifyTransaction(
                __in LogRecordLib::EndTransactionLogRecord & record,
                __in LogRecordLib::TransactionLogRecord & invalidTransactionLogRecord);
            
            // Used during CR of P->S to unlock any pending locks on the primary that is transitioning out
            void GetPendingRecords(__out KArray<LogRecordLib::TransactionLogRecord::SPtr> & pendingRecords);

            // Used during CR to Primary to abort any pending transactions that are present in the system
            void GetPendingTransactions(__out KArray<LogRecordLib::BeginTransactionOperationLogRecord::SPtr> & pendingTransactions);

            // Used during checkpoints to abort old transactions that are pending for too long
            void GetPendingTransactionsOlderThanPosition(
                __in ULONG64 recordPosition,
                __out KArray<LogRecordLib::BeginTransactionOperationLogRecord::SPtr> & pendingTransactions);

            // Used to remove transactions from the map that are stablized after barrier processing as we no longer need them
            // This releases memory consumption in the map structures
            void RemoveStableTransactions(__in LONG64 lastStableLsn);

        private:

            TransactionMap(__in Data::Utilities::PartitionedReplicaId const & traceId);
            
            void AddUnstableTransactionCallerHoldsLock(
                __in LogRecordLib::BeginTransactionOperationLogRecord & beginTransactionRecord,
                __in LogRecordLib::EndTransactionLogRecord & endTransactionRecord);
            
            struct LsnKeyType
            {
                LsnKeyType(LONG64 val)
                {
                    this->value = val;
                }

                LONG64 value;
            };
            
            struct LsnKeyTypeCmp
            {
                bool operator()(LsnKeyType const & lhs, LsnKeyType const & rhs) const
                {
                    return lhs.value < rhs.value;
                }
            };

            KSpinLock lock_;

            KArray<LogRecordLib::BeginTransactionOperationLogRecord::SPtr> completedTransactions_;

            std::unordered_map<LONG64, LogRecordLib::TransactionLogRecord::SPtr> latestRecords_;

            std::map<LsnKeyType, LogRecordLib::BeginTransactionOperationLogRecord::SPtr, LsnKeyTypeCmp> lsnPendingTransactions_;
            
            // needed to quickly find earliest pending tx. Order by lsn
            std::unordered_map<LONG64, LogRecordLib::BeginTransactionOperationLogRecord::SPtr> transactionIdPendingTransactionsPair_;

            // lsn ordered
            KArray<LogRecordLib::EndTransactionLogRecord::SPtr> unstableTransactions_;
        };
    }
}
