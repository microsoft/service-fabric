// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

Common::StringLiteral const TraceComponent("ParallelLogRecordsDispatcher");

ParallelLogRecordsDispatcher::ParallelLogRecordsDispatcher(
    __in PartitionedReplicaId const & traceId,
    __in IOperationProcessor & processor,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig)
    : LogRecordsDispatcher(
        traceId,
        processor,
        transactionalReplicatorConfig)
    , concurrentRecords_(GetThisAllocator())
    , processingIndex_(0)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"ParallelLogRecordsDispatcher",
        reinterpret_cast<uintptr_t>(this));

    THROW_ON_CONSTRUCTOR_FAILURE(concurrentRecords_);
}

ParallelLogRecordsDispatcher::~ParallelLogRecordsDispatcher()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"ParallelLogRecordsDispatcher",
        reinterpret_cast<uintptr_t>(this));
}

LogRecordsDispatcher::SPtr ParallelLogRecordsDispatcher::Create(
    __in PartitionedReplicaId const & traceId,
    __in IOperationProcessor & processor,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in KAllocator & allocator)
{
    ParallelLogRecordsDispatcher * pointer = _new(LOGRECORDS_DISPATCHER_TAG, allocator)ParallelLogRecordsDispatcher(
        traceId, 
        processor,
        transactionalReplicatorConfig);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return LogRecordsDispatcher::SPtr(pointer);
}

Awaitable<void> ParallelLogRecordsDispatcher::ProcessAtomicOperation(__in LogRecord & atomicOperation)
{
    LogRecord::SPtr localAtomicOperation = &atomicOperation;
    co_await recordsProcessor_->ProcessLoggedRecordAsync(*localAtomicOperation);
    co_return;
}

void ParallelLogRecordsDispatcher::ProcessConcurrentTransactions(__in std::unordered_map<LONG64, TransactionChainSPtr> & concurrentTransactions)
{
    for (auto & pair : concurrentTransactions)
    {
        ProcessSpawnedTransaction(pair.second);
    }
}

void ParallelLogRecordsDispatcher::ProcessConcurrentAtomicOperations(__in KArray<LogRecord::SPtr> & atomicOperations)
{
    for (auto & atomicOperation : atomicOperations)
    {
        ProcessSpawnedAtomicOperation(*atomicOperation);
    }
}

void ParallelLogRecordsDispatcher::ProcessedSpawnedTransaction()
{
    LONG64 remainingTransactions = InterlockedDecrement64(&numberOfSpawnedTransactions_);

    ASSERT_IFNOT(
        remainingTransactions >= 0,
        "{0}:ProcessedSpawnedTransaction | Invalid number of spawned transactions: {1}",
        TraceId,
        remainingTransactions);

    if (remainingTransactions == 0)
    {
        concurrentRecords_.Clear();

        if (!IsProcessingCompleted())
        {
            ScheduleStartProcessingLoggedRecords();
        }
    }
}

Awaitable<bool> ParallelLogRecordsDispatcher::ProcessLoggedRecords()
{
    NTSTATUS status = STATUS_SUCCESS;

    while (processingRecords_->Count() > 0)
    {
        bool isBarrierRecord = false;

        do
        {
            ASSERT_IFNOT(
                processingRecords_->Count() > 0,
                "{0}:ProcessLoggedRecords | Must be processing > 0 records",
                TraceId);

            LoggedRecords::CSPtr loggedRecords = (*processingRecords_)[0];

            if (NT_SUCCESS(loggedRecords->LogError))
            {
                // Copy from processingRecords_ to concurrent records until barrier

                while (processingIndex_ < loggedRecords->Count)
                {
                    LogRecord::SPtr record = (*loggedRecords)[processingIndex_];

                    isBarrierRecord = LogRecord::IsBarrierRecord(*record, transactionalReplicatorConfig_->EnableSecondaryCommitApplyAcknowledgement);

                    // If barrier is hit process records before the barrier 
                    if (isBarrierRecord)
                    {
                        // NOTE: The check for concurrentRecords count is needed to ensure we dont spin in an infinite loop when we encounter a barrier
                        if (concurrentRecords_.Count() > 0)
                        {
                            co_await PauseDispatchingIfNeededAsync(*record);
                            recordsProcessor_->UpdateDispatchingBarrierTask(*record->GetAppliedTask());
                            break;
                        }
                    }

                    ++processingIndex_;
                    status = concurrentRecords_.Append(record);
                    THROW_ON_FAILURE(status);

                    if (isBarrierRecord)
                    {
                        break;
                    }
                }
            }
            else if (concurrentRecords_.Count() > 0)
            {
                // If an exception occurred, process what we have so far
                isBarrierRecord = true;
            }
            else
            {
                // If first record is an exception, process everything with exception

                while (processingIndex_ < loggedRecords->Count)
                {
                    LogRecord::SPtr record = (*loggedRecords)[processingIndex_];
                    ++processingIndex_;

                    co_await recordsProcessor_->ImmediatelyProcessRecordAsync(
                        *record,
                        loggedRecords->LogError,
                        RecordProcessingMode::Enum::ProcessImmediately);
                }
            }

            if (processingIndex_ == loggedRecords->Count)
            {
                BOOLEAN result = processingRecords_->Remove(0);
                ASSERT_IFNOT(
                    result,
                    "{0}:ProcessLoggedRecords | Failed to remove record",
                    TraceId);

                processingIndex_ = 0;
            }

            if (isBarrierRecord)
            {
                break;
            }
        } while (processingRecords_->Count() > 0);

        // If we do have records to process, but do not hit barrier, we do not process as yet

        if (!isBarrierRecord)
        {
            ULONG count = concurrentRecords_.Count();

            if (count > 0)
            {
                EventSource::Events->DispatcherConcurrentRecords(
                    TracePartitionId,
                    ReplicaId,
                    count,
                    concurrentRecords_[0]->Psn,
                    concurrentRecords_[count - 1]->Psn);
            }

            break;
        }

        for (ULONG i = 0; i < concurrentRecords_.Count(); i++)
        {
            LogRecord::SPtr record = concurrentRecords_[i];
            RecordProcessingMode::Enum processingMode = recordsProcessor_->IdentifyProcessingModeForRecord(*record);

            // processingMode = Normal,ApplyImmediately
            if (processingMode < RecordProcessingMode::Enum::ProcessImmediately)
            {
                PrepareToProcessRecord(*record, processingMode);
            }

            // processingMode == ApplyImmediately, ProcessImmediately
            if (processingMode > RecordProcessingMode::Enum::Normal)
            {
                co_await recordsProcessor_->ImmediatelyProcessRecordAsync(*record, STATUS_SUCCESS, processingMode);
                concurrentRecords_.Remove(i);
                i--;
            }
        }

        if (concurrentRecords_.Count() == 0)
        {
            continue;
        }
        else if (concurrentRecords_.Count() == 1)
        {
            co_await recordsProcessor_->ProcessLoggedRecordAsync(*concurrentRecords_[0]);
        }
        else
        {
            LONG numberOfTransactions = co_await SeparateTransactions();

            if (numberOfTransactions > 1)
            {
                co_return false;
            }
        }

        concurrentRecords_.Clear();
    }

    co_return true;
}

Task ParallelLogRecordsDispatcher::ProcessSpawnedTransaction(__in TransactionChainSPtr & transaction)
{
    KCoShared$ApiEntry();

    co_await ProcessTransaction(transaction);

    ProcessedSpawnedTransaction();

    co_return;
}

Task ParallelLogRecordsDispatcher::ProcessSpawnedAtomicOperation(__in LogRecord & logRecord)
{
    KCoShared$ApiEntry();

    co_await ProcessAtomicOperation(logRecord);

    ProcessedSpawnedTransaction();

    co_return;
}

Awaitable<void> ParallelLogRecordsDispatcher::ProcessTransaction(__in TransactionChainSPtr & transaction)
{
    TransactionChainSPtr localTransaction = Ktl::Move(transaction);
    for (ULONG i = 0; i < localTransaction->Count(); i++)
    {
        co_await recordsProcessor_->ProcessLoggedRecordAsync(*(*localTransaction)[i]);
    }

    co_return;
}

Awaitable<LONG> ParallelLogRecordsDispatcher::SeparateTransactions()
{
    ASSERT_IFNOT(
        concurrentRecords_.Count() > 0,
        "{0}:SeparateTransactions | Concurrent records should not be empty: {1}",
        TraceId,
        concurrentRecords_.Count());

    LONG numberOfTransactions = 0;

    KArray<LogRecord::SPtr> atomicOperations(GetThisAllocator());
    THROW_ON_CONSTRUCTOR_FAILURE(atomicOperations);

    std::unordered_map<LONG64, KSharedArray<TransactionLogRecord::SPtr>::SPtr> concurrentTransactions;
    NTSTATUS status = STATUS_SUCCESS;

    for (ULONG i = 0; i < concurrentRecords_.Count(); i++)
    {
        LogRecord::SPtr & record = concurrentRecords_[i];

        OperationLogRecord::CSPtr operationLogRecord = dynamic_cast<OperationLogRecord *>(record.RawPtr());
        TransactionLogRecord::SPtr transactionLogRecord = dynamic_cast<TransactionLogRecord *>(record.RawPtr());

        if (transactionLogRecord == nullptr ||
            (operationLogRecord != nullptr && operationLogRecord->BaseTransaction.IsAtomicOperation))
        {
            // This is an atomic operation. Add to the list to dispatch them in parallel

            ++numberOfTransactions;
            status = atomicOperations.Append(record);
            THROW_ON_FAILURE(status);
        }
        else
        {
            // This is a transaction
            LONG64 txId = transactionLogRecord->BaseTransaction.TransactionId;
            KSharedArray<TransactionLogRecord::SPtr>::SPtr transaction = nullptr;

            if (concurrentTransactions.count(txId) > 0)
            {
                transaction = concurrentTransactions[txId];
                ASSERT_IFNOT(
                    transaction != nullptr,
                    "{0}:ParallelLogRecordsDispatcher::SeparateTransactions | Transaction must not be null",
                    TraceId);

                status = transaction->Append(transactionLogRecord);
                ASSERT_IFNOT(
                    status == STATUS_SUCCESS,
                    "{0}:ParallelLogRecordsDispatcher::SeparateTransactions | Failed to append transaction log record",
                    TraceId);
            }
            else
            {
                KSharedArray<TransactionLogRecord::SPtr> * txPointer = _new(LOGRECORDS_DISPATCHER_TAG, GetThisAllocator())KSharedArray<TransactionLogRecord::SPtr>();
                THROW_ON_ALLOCATION_FAILURE(txPointer);

                transaction = txPointer;

                status = transaction->Append(transactionLogRecord);
                ASSERT_IFNOT(
                    status == STATUS_SUCCESS,
                    "{0}:ParallelLogRecordsDispatcher::SeparateTransactions | Failed to append transaction log record",
                    TraceId);
                
                concurrentTransactions[txId] = transaction;
                ++numberOfTransactions;
            }
        }
    }

    ASSERT_IFNOT(
        numberOfTransactions > 0,
        "{0}:ParallelLogRecordsDispatcher::SeparateTransactions | Invalid number of transactions: {1}",
        TraceId,
        numberOfTransactions);

    if (numberOfTransactions == 1)
    {
        // await in this IF block because there is only 1 transaction to process and the caller continues the processing after processing this transaction/atomic operation

        if (atomicOperations.Count() != 0)
        {
            co_await ProcessAtomicOperation(*atomicOperations[0]);
        }
        else
        {
            auto it = concurrentTransactions.begin();
            ASSERT_IFNOT(
                it != concurrentTransactions.end(),
                "{0}:ParallelLogRecordsDispatcher::SeperateTransactions | Expected 1 concurrent transactions, found {1}",
                TraceId,
                concurrentTransactions.size());

            co_await ProcessTransaction(it->second);
        }
    }
    else
    {
        // fire and forget in this IF block because there are multiple transactions to process and the caller stops the processing after this method returns.
        // the last transaction that completes processing triggers the next call to begin processing again

        numberOfSpawnedTransactions_ = numberOfTransactions;

        if (atomicOperations.Count() != 0)
        {
            ProcessConcurrentAtomicOperations(atomicOperations);
        }

        if (concurrentTransactions.size() != 0)
        {
            ProcessConcurrentTransactions(concurrentTransactions);
        }
    }

    co_return numberOfTransactions;
}
