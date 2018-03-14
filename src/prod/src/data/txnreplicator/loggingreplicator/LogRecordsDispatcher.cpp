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

Common::StringLiteral const TraceComponent("LogRecordsDispatcher");

LogRecordsDispatcher::LogRecordsDispatcher(
    __in PartitionedReplicaId const & traceId,
    __in IOperationProcessor & processor,
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , enableSecondaryCommitApplyAcknowledgement_(transactionalReplicatorConfig->EnableSecondaryCommitApplyAcknowledgement)
    , dispatchLock_()
    , concurrentRecords_(GetThisAllocator())
    , flushedRecords_()
    , numberOfSpawnedTransactions_(0)
    , processingIndex_(0)
    , processingRecords_()
    , recordsProcessor_(&processor)
    , dispatchPauseAtBarrierLock_()
    , lastDispatchingBarrier_()
    , dispatchPauseAtBarrierTcs_()
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"LogRecordsDispatcher",
        reinterpret_cast<uintptr_t>(this));

    THROW_ON_CONSTRUCTOR_FAILURE(concurrentRecords_);
}

LogRecordsDispatcher::~LogRecordsDispatcher()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"LogRecordsDispatcher",
        reinterpret_cast<uintptr_t>(this));
}

LogRecordsDispatcher::SPtr LogRecordsDispatcher::Create(
    __in PartitionedReplicaId const & traceId,
    __in IOperationProcessor & processor,
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in KAllocator & allocator)
{
    LogRecordsDispatcher * pointer = _new(LOGRECORDS_DISPATCHER_TAG, allocator)LogRecordsDispatcher(
        traceId, 
        processor,
        transactionalReplicatorConfig);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return LogRecordsDispatcher::SPtr(pointer);
}

void LogRecordsDispatcher::DispatchLoggedRecords(__in LoggedRecords const & loggedRecords)
{
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(dispatchLock_)
    {
        if (processingRecords_ != nullptr)
        {
            if (flushedRecords_ == nullptr)
            {
                flushedRecords_ = _new(LOGRECORDS_DISPATCHER_TAG, GetThisAllocator())KSharedArray<LoggedRecords::CSPtr>();
                THROW_ON_ALLOCATION_FAILURE(flushedRecords_);
            }

            status = flushedRecords_->Append(&loggedRecords);
            THROW_ON_FAILURE(status);
            return;
        }

        ASSERT_IFNOT(
            flushedRecords_ == nullptr,
            "{0} : LogRecordsDispatcher::DispatchLoggedRecords | flushedRecords_ must be null",
            TraceId);

        ASSERT_IFNOT(
            processingIndex_ == 0,
            "{0} : LogRecordsDispatcher::DispatchLoggedRecords | processingIndex_ must be 0, current value = {1}",
            TraceId,
            processingIndex_);

        processingRecords_ = _new(LOGRECORDS_DISPATCHER_TAG, GetThisAllocator())KSharedArray<LoggedRecords::CSPtr>();
        THROW_ON_ALLOCATION_FAILURE(processingRecords_);

        status = processingRecords_->Append(&loggedRecords);
        THROW_ON_FAILURE(status);
    }

    ScheduleStartProcessingLoggedRecords();
}

void LogRecordsDispatcher::ScheduleStartProcessingLoggedRecords()
{
    KThreadPool & threadPool = GetThisAllocator().GetKtlSystem().DefaultSystemThreadPool();

    // Add ref on itself to ensure object does not go away while executing the background work item
    AddRef();

    // The TRUE here is required to ensure that we don't run into stack overflow
    threadPool.QueueWorkItem(*this, TRUE); 
}

void LogRecordsDispatcher::Execute()
{
    // Release the reference taken before starting the work item
    KFinally([&] { this->Release(); });

    StartProcessingLoggedRecords();
}

bool LogRecordsDispatcher::IsProcessingCompleted()
{
    K_LOCK_BLOCK(dispatchLock_)
    {
        if (processingRecords_->Count() != 0)
        {
            return false;
        }

        processingRecords_ = flushedRecords_;

        if (processingRecords_ == nullptr)
        {
            return true;
        }

        flushedRecords_ = nullptr;
    }

    return false;
}

void LogRecordsDispatcher::PrepareToProcessRecord(
    __in LogRecord & record,
    __in RecordProcessingMode::Enum processingMode)
{
    ASSERT_IFNOT(
        processingMode < RecordProcessingMode::Enum::ProcessImmediately,
        "{0}:PrepareToProcessRecord Processing mode is unexpectedly process immediately",
        TraceId);

    LogicalLogRecord * logicalRecord = record.AsLogicalLogRecord();
    PhysicalLogRecord * physicalRecord = record.AsPhysicalLogRecord();

    ASSERT_IFNOT(
        logicalRecord != nullptr || physicalRecord != nullptr,
        "{0}:PrepareToProcessRecord | logicalRecord and physicalRecord must not be null",
        TraceId);

    if (logicalRecord != nullptr)
    {
        recordsProcessor_->PrepareToProcessLogicalRecord();
    }
    else
    {
        recordsProcessor_->PrepareToProcessPhysicalRecord();
    }
}
        
Awaitable<void> LogRecordsDispatcher::ProcessAtomicOperations(__in KArray<LogRecord::SPtr> & atomicOperations)
{
    KArray<LogRecord::SPtr> localAtomicOperations = Ktl::Move(atomicOperations);
    THROW_ON_CONSTRUCTOR_FAILURE(localAtomicOperations);

    KArray<Awaitable<void>> tasks(GetThisAllocator());
    THROW_ON_CONSTRUCTOR_FAILURE(tasks);

    NTSTATUS status = STATUS_SUCCESS;

    for (ULONG i = 0; i < localAtomicOperations.Count(); i++)
    {
        Awaitable<void> task = recordsProcessor_->ProcessLoggedRecordAsync(*localAtomicOperations[i]);
        status = tasks.Append(Ktl::Move(task));
        THROW_ON_FAILURE(status);
    }

    co_await TaskUtilities<void>::WhenAll(tasks);

    co_return;
}

void LogRecordsDispatcher::ProcessConcurrentTransactions(__in std::unordered_map<LONG64, TransactionChainSPtr> & concurrentTransactions)
{
    for (auto & pair : concurrentTransactions)
    {
        ProcessSpawnedTransaction(pair.second);
    }
}

void LogRecordsDispatcher::ProcessedSpawnedTransaction()
{
    LONG64 remainingTransactions = InterlockedDecrement64(&numberOfSpawnedTransactions_);

    ASSERT_IFNOT(
        remainingTransactions >= 0,
        "{0}:ProcessSpawnedTransaction | Invalid number of spawned transactions: {1}",
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

Awaitable<bool> LogRecordsDispatcher::ProcessLoggedRecords()
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

                    isBarrierRecord = LogRecord::IsBarrierRecord(*record, enableSecondaryCommitApplyAcknowledgement_);

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

Task LogRecordsDispatcher::ProcessSpawnedAtomicOperations(__in KArray<LogRecord::SPtr> & atomicOperations)
{
    // Since this method could run in the background, ensure we have taken a reference
    KCoShared$ApiEntry()

    co_await ProcessAtomicOperations(atomicOperations);

    ProcessedSpawnedTransaction();

    co_return;
}

Task LogRecordsDispatcher::ProcessSpawnedTransaction(__in TransactionChainSPtr & transaction)
{
    // Since this method could run in the background, ensure we have taken a reference
    KCoShared$ApiEntry()

    co_await ProcessTransaction(transaction);

    ProcessedSpawnedTransaction();

    co_return;
}

Awaitable<void> LogRecordsDispatcher::ProcessTransaction(__in TransactionChainSPtr & transaction)
{
    TransactionChainSPtr localTransaction = Ktl::Move(transaction);
    for (ULONG i = 0; i < localTransaction->Count(); i++)
    {
        co_await recordsProcessor_->ProcessLoggedRecordAsync(*(*localTransaction)[i]);
    }

    co_return;
}

Awaitable<LONG> LogRecordsDispatcher::SeparateTransactions()
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
                    "{0}:LogRecordsDispatcher::SeparateTransactions | Transaction must not be null",
                    TraceId);

                status = transaction->Append(transactionLogRecord);
                ASSERT_IFNOT(
                    status == STATUS_SUCCESS,
                    "{0}:LogRecordsDispatcher::SeparateTransactions | Failed to append transaction log record",
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
                    "{0}:LogRecordsDispatcher::SeparateTransactions | Failed to append transaction log record",
                    TraceId);
                
                concurrentTransactions[txId] = transaction;
                ++numberOfTransactions;
            }
        }
    }

    ASSERT_IFNOT(
        numberOfTransactions > 0,
        "{0}:LogRecordsDispatcher::SeparateTransactions | Invalid number of transactions: {1}",
        TraceId,
        numberOfTransactions);

    if (numberOfTransactions == 1)
    {
        // await in this IF block because there is only 1 transaction to process and the caller continues the processing after processing this transaction/atomic operation

        if (atomicOperations.Count() != 0)
        {
            co_await ProcessAtomicOperations(atomicOperations);
        }
        else
        {
            auto it = concurrentTransactions.begin();
            ASSERT_IFNOT(
                it != concurrentTransactions.end(),
                "{0}:LogRecordsDispatcher::SeperateTransactions | Expected 1 concurrent transactions, found {1}",
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
            ProcessSpawnedAtomicOperations(atomicOperations);
        }

        if (concurrentTransactions.size() != 0)
        {
            ProcessConcurrentTransactions(concurrentTransactions);
        }
    }

    co_return numberOfTransactions;
}

Task LogRecordsDispatcher::StartProcessingLoggedRecords()
{
    // Since this method could run in the background, ensure we have taken a reference
    KCoShared$ApiEntry()

    while (true)
    {
        ASSERT_IFNOT(
            processingRecords_->Count() > 0,
            "{0}:StartProcessingLoggedRecords | No processing records in log records dispatcher: {1}",
            TraceId,
            processingRecords_->Count());

        LONGLONG initialValue = InterlockedCompareExchange64(&numberOfSpawnedTransactions_, 0, 0);

        ASSERT_IFNOT(
            initialValue == 0,
            "{0}:StartProcessingLoggedRecords | Invalid number of spawned transaction count: {1}",
            TraceId,
            initialValue);

        bool continueProcessing = co_await ProcessLoggedRecords();

        if (!continueProcessing)
        {
            co_return;
        }

        ASSERT_IFNOT(
            processingRecords_->Count() == 0,
            "{0}:StartProcessingLoggedRecords | Log records still found after processing complete: {1}",
            TraceId,
            processingRecords_->Count());

        bool isProcessingCompleted = IsProcessingCompleted();

        if (isProcessingCompleted)
        {
            break;
        }
    }

    co_return;
}

Awaitable<NTSTATUS> LogRecordsDispatcher::DrainAndPauseDispatchAsync()
{
    LogRecord::SPtr snapLastDispatchingBarrier = nullptr;
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(dispatchPauseAtBarrierLock_)
    {
        snapLastDispatchingBarrier = lastDispatchingBarrier_;

        ASSERT_IFNOT(
            dispatchPauseAtBarrierTcs_ == nullptr,
            "{0}:Cannot DrainAndPauseDispatch more than once concurrently",
            TraceId);
    
        dispatchPauseAtBarrierTcs_ = CompletionTask::CreateAwaitableCompletionSource<void>(LOGRECORDS_DISPATCHER_TAG, GetThisAllocator());
    }

    if (snapLastDispatchingBarrier != nullptr)
    {
        EventSource::Events->DispatcherDrainStart(
            TracePartitionId,
            ReplicaId,
            snapLastDispatchingBarrier->Lsn,
            snapLastDispatchingBarrier->Psn);

        status = co_await snapLastDispatchingBarrier->AwaitApply();

        EventSource::Events->DispatcherDrainFinish(
            TracePartitionId,
            ReplicaId,
            snapLastDispatchingBarrier->Lsn,
            snapLastDispatchingBarrier->Psn,
            status);
    }

    co_return status;
}

void LogRecordsDispatcher::ContinueDispatch()
{
    AwaitableCompletionSource<void>::SPtr continueTcs = nullptr;

    K_LOCK_BLOCK(dispatchPauseAtBarrierLock_)
    {
        ASSERT_IF(
            dispatchPauseAtBarrierTcs_ == nullptr,
            "{0}:Cannot ContinueDispatch more than once concurrently",
            TraceId);
        
        continueTcs = dispatchPauseAtBarrierTcs_;
        dispatchPauseAtBarrierTcs_ = nullptr;
    }

    continueTcs->Set();
}

Awaitable<void> LogRecordsDispatcher::PauseDispatchingIfNeededAsync(LogRecord & logRecord)
{
    LogRecord::SPtr logRecordSPtr = &logRecord;
    AwaitableCompletionSource<void>::SPtr waitTcs = nullptr;

    K_LOCK_BLOCK(dispatchPauseAtBarrierLock_)
    {
        waitTcs = dispatchPauseAtBarrierTcs_;
        // Update the last dispatching barrier only if we are going to dispatch the applies for this barrier
        if (waitTcs == nullptr)
        {
            lastDispatchingBarrier_ = logRecordSPtr;
        }
    }

    if (waitTcs == nullptr)
    {
        EventSource::Events->DispatcherBarrier(
            TracePartitionId,
            ReplicaId,
            logRecordSPtr->Lsn,
            logRecordSPtr->Psn);

        co_return;
    }

    EventSource::Events->DispatcherPauseAtBarrier(
        TracePartitionId,
        ReplicaId,
        logRecordSPtr->Lsn,
        logRecordSPtr->Psn);

    co_await waitTcs->GetAwaitable();

    EventSource::Events->DispatcherContinue(
        TracePartitionId,
        ReplicaId,
        logRecordSPtr->Lsn,
        logRecordSPtr->Psn);

    // Update the last Dispatching barrier task after the pause has ended
    K_LOCK_BLOCK(dispatchPauseAtBarrierLock_)
    {
        lastDispatchingBarrier_ = logRecordSPtr;
    }

    co_return;
}

