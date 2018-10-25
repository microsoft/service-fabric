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
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , processingRecords_()
    , recordsProcessor_(&processor)
    , numberOfSpawnedTransactions_(0)
    , dispatchLock_()
    , flushedRecords_()
    , dispatchPauseAtBarrierLock_()
    , lastDispatchingBarrier_()
    , dispatchPauseAtBarrierTcs_()
{
}

LogRecordsDispatcher::~LogRecordsDispatcher()
{
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

    threadPool.QueueWorkItem(*this); 
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
