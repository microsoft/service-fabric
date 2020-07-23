// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Log;
using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

Common::StringLiteral const TraceComponent("PhysicalLogWriter");

PhysicalLogWriter::PhysicalLogWriter(
    __in PartitionedReplicaId const & traceId,
    __in ILogicalLog & logicalLogStream,
    __in PhysicalLogWriterCallbackManager & callbackManager,
    __in LONG maxWriteCacheSizeBytes,
    __in bool recomputeOffsets,
    __in LogRecord & invalidTailRecord,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , isDisposed_(false)
    , flushNotificationLock_()
    , completedFlushNotificationTcs_()
    , flushNotificationTcs_()
    , flushLock_()
    , closedError_(0)
    , loggingError_(0)
    , isCallbackManagerReset_(false)
    , maxWriteCacheSizeInBytes_(maxWriteCacheSizeBytes)
    , bufferedRecordsBytes_(0)
    , pendingFlushRecordsBytes_(0)
    , currentLogTailPosition_(0)
    , currentLogTailPsn_(0)
    , currentLogTailRecord_(&invalidTailRecord)
    , logicalLogStream_(&logicalLogStream)
    , callbackManager_(&callbackManager)
    , bufferedRecords_(nullptr)
    , flushingRecords_(nullptr)
    , pendingFlushRecords_(nullptr)
    , pendingFlushTasks_(nullptr)
    , recordWriter_(GetThisAllocator())
    , perfCounters_(perfCounters)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , runningLatencySumMs_(0)
    , writeSpeedBytesPerSecondSum_(0)
    , avgRunningLatencyMilliseconds_(GetThisAllocator(), Constants::PhysicalLogWriterMovingAverageHistory, 0)
    , avgWriteSpeedBytesPerSecond_(GetThisAllocator(), Constants::PhysicalLogWriterMovingAverageHistory, 0)
    , recomputeOffsets_(recomputeOffsets)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"PhysicalLogWriter",
        reinterpret_cast<uintptr_t>(this));

    ioMonitor_ = IOMonitor::Create(
        traceId,
        Constants::SlowPhysicalLogWriteOperationName,
        Common::SystemHealthReportCode::TR_SlowIO,
        transactionalReplicatorConfig,
        healthClient,
        GetThisAllocator());

    ASSERT_IFNOT(ioMonitor_ != nullptr, "Failed to initialize health tracker");

    THROW_ON_CONSTRUCTOR_FAILURE(avgRunningLatencyMilliseconds_);
    THROW_ON_CONSTRUCTOR_FAILURE(avgWriteSpeedBytesPerSecond_);

    InitializeMovingAverageKArray();

    ASSERT_IFNOT(
        currentLogTailRecord_.Get()->RecordPosition == Constants::InvalidRecordPosition,
        "{0}: Valid current log tail record position encountered in constructor: {1}",
        TraceId,
        currentLogTailRecord_.Get()->RecordPosition);

    InitializeCompletedTcs(completedFlushNotificationTcs_, GetThisAllocator());
    flushNotificationTcs_ = completedFlushNotificationTcs_;

    currentLogTailPosition_ = static_cast<ULONG64>(logicalLogStream_->GetWritePosition());
}

PhysicalLogWriter::PhysicalLogWriter(
    __in PartitionedReplicaId const & traceId,
    __in ILogicalLog & logicalLogStream,
    __in PhysicalLogWriterCallbackManager & callbackManager,
    __in LONG maxWriteCacheSizeBytes,
    __in bool recomputeOffsets,
    __in LogRecord & tailRecord,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in NTSTATUS closedError)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , isDisposed_(false)
    , flushNotificationLock_()
    , completedFlushNotificationTcs_()
    , flushNotificationTcs_()
    , flushLock_()
    , closedError_(closedError)
    , loggingError_(0)
    , isCallbackManagerReset_(false)
    , maxWriteCacheSizeInBytes_(maxWriteCacheSizeBytes)
    , bufferedRecordsBytes_(0)
    , pendingFlushRecordsBytes_(0)
    , currentLogTailPosition_(0)
    , currentLogTailPsn_(0)
    , currentLogTailRecord_(&tailRecord)
    , logicalLogStream_(&logicalLogStream)
    , callbackManager_(&callbackManager)
    , bufferedRecords_(nullptr)
    , flushingRecords_(nullptr)
    , pendingFlushRecords_(nullptr)
    , pendingFlushTasks_(nullptr)
    , recordWriter_(GetThisAllocator())
    , perfCounters_(perfCounters)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , runningLatencySumMs_(0)
    , writeSpeedBytesPerSecondSum_(0)
    , avgRunningLatencyMilliseconds_(GetThisAllocator(), Constants::PhysicalLogWriterMovingAverageHistory, 0)
    , avgWriteSpeedBytesPerSecond_(GetThisAllocator(), Constants::PhysicalLogWriterMovingAverageHistory, 0)
    , recomputeOffsets_(recomputeOffsets)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"PhysicalLogWriter",
        reinterpret_cast<uintptr_t>(this));

    THROW_ON_CONSTRUCTOR_FAILURE(avgRunningLatencyMilliseconds_);
    THROW_ON_CONSTRUCTOR_FAILURE(avgWriteSpeedBytesPerSecond_);

    ioMonitor_ = IOMonitor::Create(
        traceId,
        Constants::SlowPhysicalLogWriteOperationName,
        Common::SystemHealthReportCode::TR_SlowIO,
        transactionalReplicatorConfig,
        healthClient,
        GetThisAllocator());

    ASSERT_IFNOT(ioMonitor_ != nullptr, "Failed to initialize health tracker");

    InitializeMovingAverageKArray();

    InitializeCompletedTcs(completedFlushNotificationTcs_, GetThisAllocator());
    flushNotificationTcs_ = completedFlushNotificationTcs_;

    SetTailRecord(tailRecord);

    ASSERT_IFNOT(
        currentLogTailPosition_ == static_cast<ULONG64>(logicalLogStream_->GetWritePosition()),
        "{0}: Current log tail position: {1} != logical log stream write position: {2} in physical log writer constructor",
        TraceId,
        currentLogTailPosition_,
        logicalLogStream_->GetWritePosition());
}

PhysicalLogWriter::~PhysicalLogWriter()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"PhysicalLogWriter",
        reinterpret_cast<uintptr_t>(this));

    ASSERT_IFNOT(
        isDisposed_,
        "{0}: Physical log writter object not disposed during destructor",
        TraceId);
}

void PhysicalLogWriter::Dispose()
{
    if (isDisposed_)
    {
        return;
    }

    ASSERT_IFNOT(
        IsCompletelyFlushed,
        "{0}: Flush not complete during dispose of physical log writer",
        TraceId);

    if (!isCallbackManagerReset_ && callbackManager_ != nullptr)
    {
        callbackManager_->Dispose();
    }

    isDisposed_ = true;
}

PhysicalLogWriter::SPtr PhysicalLogWriter::Create(
    __in PartitionedReplicaId const & traceId,
    __in ILogicalLog & logicalLogStream,
    __in PhysicalLogWriterCallbackManager & callbackManager,
    __in LONG maxWriteCacheSizeBytes,
    __in bool recomputeOffsets,
    __in LogRecord & invalidTailRecord,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in KAllocator & allocator)
{
    PhysicalLogWriter * pointer = _new(PHYSICALLOGWRITER_TAG, allocator)PhysicalLogWriter(
        traceId,
        logicalLogStream,
        callbackManager,
        maxWriteCacheSizeBytes,
        recomputeOffsets,
        invalidTailRecord,
        perfCounters,
        healthClient,
        transactionalReplicatorConfig);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return PhysicalLogWriter::SPtr(pointer);
}

PhysicalLogWriter::SPtr PhysicalLogWriter::Create(
    __in PartitionedReplicaId const & traceId,
    __in ILogicalLog & logicalLogStream,
    __in PhysicalLogWriterCallbackManager & callbackManager,
    __in LONG maxWriteCacheSizeBytes,
    __in bool recomputeOffsets,
    __in LogRecord & tailRecord,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in NTSTATUS closedError,
    __in KAllocator & allocator)
{
    PhysicalLogWriter * pointer = _new(PHYSICALLOGWRITER_TAG, allocator)PhysicalLogWriter(
        traceId,
        logicalLogStream,
        callbackManager,
        maxWriteCacheSizeBytes,
        recomputeOffsets,
        tailRecord,
        perfCounters,
        healthClient,
        transactionalReplicatorConfig,
        closedError);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return PhysicalLogWriter::SPtr(pointer);
}

void PhysicalLogWriter::InitializeCompletedTcs(
    __out AwaitableCompletionSource<void>::SPtr & tcs, 
    __in KAllocator & allocator)
{
    tcs = CompletionTask::CreateAwaitableCompletionSource<void>(PHYSICALLOGWRITER_TAG, allocator);
    bool isSet = tcs->TrySet();
    ASSERT_IFNOT(
        isSet,
        "InitializeCompletedTcs | Failed to set CompletionTask");
}

void PhysicalLogWriter::WakeupFlushWaiters(__in_opt KSharedArray<AwaitableCompletionSource<void>::SPtr> const * const waitingTasks)
{
    if (waitingTasks != nullptr)
    {
        for (ULONG i = 0; i < waitingTasks->Count(); i++)
        {
            bool isSet = (*waitingTasks)[i]->TrySet();
            ASSERT_IFNOT(
                isSet,
                "WakeupFlushWaiters | Failed to set waiting task status");
        }
    }
}

LONG64 PhysicalLogWriter::InsertBufferedRecord(__in LogRecord & logRecord)
{
    PhysicalLogRecord * physicalRecord = logRecord.AsPhysicalLogRecord();
    LONG64 newBufferedRecordsBytes = 0;

    K_LOCK_BLOCK(flushLock_)
    {
        // If we are closing, fail the insert by invoking the callback with an exception
        if (!NT_SUCCESS(closedError_.load()))
        {
            LoggedRecords::SPtr failedRecords = LoggedRecords::Create(logRecord, closedError_.load(), GetThisAllocator());
            ProcessFlushedRecords(*failedRecords);

            return bufferedRecordsBytes_.load();
        }

        // Ensure that no record can be inserted after 'RemovingState' Information log record
        if (lastPhysicalRecord_ != nullptr &&
            lastPhysicalRecord_->RecordType == LogRecordType::Enum::Information)
        {
            InformationLogRecord const * lastInformationRecord = dynamic_cast<InformationLogRecord const *>(lastPhysicalRecord_.RawPtr());

            ASSERT_IFNOT(
                lastInformationRecord->InformationEvent != InformationEvent::Enum::RemovingState,
                "{0}: Invalid removing state last information event encountered",
                TraceId);
        }

        // Update the record and tail
        ++currentLogTailPsn_;
        logRecord.Psn = currentLogTailPsn_;
        logRecord.PreviousPhysicalRecord = lastPhysicalRecord_.RawPtr();

        perfCounters_->IncomingBytesPerSecond.IncrementBy(logRecord.ApproximateSizeOnDisk);

        // if this is a physical record, update the last physical
        if (physicalRecord != nullptr)
        {
            if (lastPhysicalRecord_ != nullptr)
            {
                lastPhysicalRecord_->NextPhysicalRecord = physicalRecord;
            }

            lastPhysicalRecord_ = physicalRecord;
        }

        // Buffer the record
        if (bufferedRecords_ == nullptr)
        {
            bufferedRecords_ = _new(PHYSICALLOGWRITER_TAG, GetThisAllocator())KSharedArray<LogRecord::SPtr>();
            THROW_ON_ALLOCATION_FAILURE(bufferedRecords_);
        }

        NTSTATUS status = bufferedRecords_->Append(&logRecord);
        THROW_ON_FAILURE(status);

        newBufferedRecordsBytes = bufferedRecordsBytes_.fetch_add(logRecord.ApproximateSizeOnDisk);
        newBufferedRecordsBytes += logRecord.ApproximateSizeOnDisk;
    }

    return newBufferedRecordsBytes;
}

Awaitable<void> PhysicalLogWriter::FlushAsync(KStringView const initiator)
{
    bool isFlushTask = false;
    bool isFlushing = false;
    AwaitableCompletionSource<void>::SPtr tcs = nullptr;
    LogRecord::SPtr flushWaitRecord = nullptr;
    NTSTATUS status = STATUS_SUCCESS;

    EventSource::Events->FlushInvoke(
        TracePartitionId,
        ReplicaId,
        ToStringLiteral(initiator));

    K_LOCK_BLOCK(flushLock_)
    {
        if (flushingRecords_ == nullptr)
        {
            flushingRecords_ = bufferedRecords_;

            if (flushingRecords_ != nullptr)
            {
                bufferedRecords_ = nullptr;

#ifdef DBG
                LONG cachedPendingFlushRecordsBytes = pendingFlushRecordsBytes_.load();
                ASSERT_IFNOT(
                    cachedPendingFlushRecordsBytes == 0,
                    "{0}:PendingFlushRecordsBytes must be 0. It is {1}",
                    TraceId,
                    cachedPendingFlushRecordsBytes);
#endif
                pendingFlushRecordsBytes_.store(bufferedRecordsBytes_.load());
                bufferedRecordsBytes_.store(0);
                isFlushing = true;
                isFlushTask =
                    NT_SUCCESS(loggingError_.load()) ?
                    true :
                    false;

                flushWaitRecord = (*flushingRecords_)[flushingRecords_->Count() - 1];
            }
            else
            {
                flushWaitRecord = currentLogTailRecord_.Get();
            }
        }
        else
        {
            tcs = CompletionTask::CreateAwaitableCompletionSource<void>(PHYSICALLOGWRITER_TAG, GetThisAllocator());

            if (pendingFlushRecords_ != nullptr)
            {
                if (bufferedRecords_ != nullptr)
                {
                    for (ULONG i = 0; i < bufferedRecords_->Count(); i++)
                    {
                        status = pendingFlushRecords_->Append((*bufferedRecords_)[i]);
                        THROW_ON_FAILURE(status);
                    }
                }

                ASSERT_IFNOT(
                    pendingFlushTasks_ != nullptr,
                    "{0}:FlushAsync | Pending flush tasks must exist",
                    TraceId);
            }
            else
            {
                pendingFlushRecords_ = bufferedRecords_;

                if (pendingFlushTasks_ == nullptr)
                {
                    pendingFlushTasks_ = _new(PHYSICALLOGWRITER_TAG, GetThisAllocator())KSharedArray<AwaitableCompletionSource<void>::SPtr>();
                    THROW_ON_ALLOCATION_FAILURE(pendingFlushTasks_);
                }
            }

#ifdef DBG
            LONG cachedPendingFlushRecordsBytes = pendingFlushRecordsBytes_.load();
            ASSERT_IFNOT(
                cachedPendingFlushRecordsBytes > 0,
                "{0}:PendingFlushRecordsBytes must be 0. It is {1}",
                TraceId,
                cachedPendingFlushRecordsBytes);
#endif

            bufferedRecords_ = nullptr;
            
            LONG bufferedBytes = bufferedRecordsBytes_.load();

            LONG pendingFlushRecordsBytes = pendingFlushRecordsBytes_.fetch_add(bufferedBytes);
            pendingFlushRecordsBytes += bufferedBytes;
            
            if (pendingFlushRecordsBytes > maxWriteCacheSizeInBytes_)
            {
                EventSource::Events->PendingFlushWarning(
                    TracePartitionId,
                    ReplicaId,
                    pendingFlushRecordsBytes,
                    maxWriteCacheSizeInBytes_);
            }

            bufferedRecordsBytes_.store(0);

            status = pendingFlushTasks_->Append(tcs);
            THROW_ON_FAILURE(status);

            if (pendingFlushRecords_ != nullptr)
            {
                flushWaitRecord = (*pendingFlushRecords_)[pendingFlushRecords_->Count() - 1];
            }
            else
            {
                flushWaitRecord = (*flushingRecords_)[flushingRecords_->Count() - 1];
            }
        }
    }

    if (tcs != nullptr)
    {
        co_await tcs->GetAwaitable();
    }
    else if (isFlushing)
    {
        if (isFlushTask)
        {
            AwaitableCompletionSource<void>::SPtr initiatingTcs = CompletionTask::CreateAwaitableCompletionSource<void>(PHYSICALLOGWRITER_TAG, GetThisAllocator());

            FlushTask(*initiatingTcs);

            co_await initiatingTcs->GetAwaitable();
        }
        else
        {
            KSharedArray<AwaitableCompletionSource<void>::SPtr>::SPtr emptyArray = _new(PHYSICALLOGWRITER_TAG, GetThisAllocator())KSharedArray<AwaitableCompletionSource<void>::SPtr>();
            THROW_ON_ALLOCATION_FAILURE(emptyArray);
            FailedFlushTask(emptyArray);
        }
    }
    else
    {
        co_return;
    }
    
    EventSource::Events->FlushAsync(
        TracePartitionId,
        ReplicaId,
        ToStringLiteral(initiator),
        flushWaitRecord->RecordType,
        flushWaitRecord->Lsn,
        flushWaitRecord->Psn,
        flushWaitRecord->RecordPosition);

    co_return;
}

void PhysicalLogWriter::FlushCompleted()
{
    AwaitableCompletionSource<void>::SPtr notificationTcs = nullptr;

    K_LOCK_BLOCK(flushNotificationLock_)
    {
        notificationTcs = flushNotificationTcs_;
        flushNotificationTcs_ = completedFlushNotificationTcs_;
    }
    
    // Signal completion only if there was a valid listener
    if (notificationTcs != nullptr &&
        notificationTcs != completedFlushNotificationTcs_)
    {
        bool isSet = notificationTcs->TrySet();
        ASSERT_IFNOT(
            isSet,
            "{0}:FlushCompleted | Failed to set notification tcs value",
            TraceId);
    }
}

void PhysicalLogWriter::FlushStarting()
{
    K_LOCK_BLOCK(flushNotificationLock_)
    {
        ASSERT_IFNOT(
            flushNotificationTcs_ == completedFlushNotificationTcs_,
            "{0}:FlushStarting | flushNotificationTcs_ must equal completedFlushNotificationTcs_",
            TraceId);

        flushNotificationTcs_ = nullptr;
    }
}

AwaitableCompletionSource<void>::SPtr PhysicalLogWriter::GetFlushCompletionTask()
{
    AwaitableCompletionSource<void>::SPtr tcs = nullptr;

    K_LOCK_BLOCK(flushNotificationLock_)
    {
        if (flushNotificationTcs_ == nullptr)
        {
            // This is the first call while the flush is pending. create a new tcs and return it to the caller
            flushNotificationTcs_ = CompletionTask::CreateAwaitableCompletionSource<void>(PHYSICALLOGWRITER_TAG, GetThisAllocator());
        }

        tcs = flushNotificationTcs_;
    }
    
    return tcs;
}

Task PhysicalLogWriter::FlushTask(__in AwaitableCompletionSource<void> & initiatingTcs)
{
    // This is to ensure this background task keeps the physicallogwriter object alive
    KCoShared$ApiEntry()

    KSharedArray<AwaitableCompletionSource<void>::SPtr>::SPtr flushingTasks = _new(PHYSICALLOGWRITER_TAG, GetThisAllocator())KSharedArray<AwaitableCompletionSource<void>::SPtr>();
    THROW_ON_ALLOCATION_FAILURE(flushingTasks);

    NTSTATUS status = flushingTasks->Append(&initiatingTcs);
    THROW_ON_FAILURE(status);

    bool isFlushTask = true;
    LogRecord::SPtr newTail = nullptr;
    Common::Stopwatch flushWatch;
    flushWatch.Start();

    Common::Stopwatch serializationWatch;
    serializationWatch.Start();

    do
    {
        ASSERT_IFNOT(
            NT_SUCCESS(loggingError_.load()),
            "{0}:FlushTask | Unexpected logging exception before starting FlushTask",
            TraceId);

        ULONG latencySensitiveRecords = 0;
        ULONG numberOfBytes = 0;

        EventSource::Events->FlushStart(
            TracePartitionId,
            ReplicaId,
            flushingRecords_->Count(),
            (*flushingRecords_)[0]->Psn,
            logicalLogStream_->GetWritePosition());

        // Enable group commit as we are about to issue IO
        FlushStarting();

        flushWatch.Reset();
        serializationWatch.Reset();

        for (ULONG i = 0; i < flushingRecords_->Count(); i++)
        {
            serializationWatch.Start();

            OperationData::CSPtr operationData = WriteRecord(*(*flushingRecords_)[i], numberOfBytes);

#ifdef DBG
            ReplicatedLogManager::ValidateOperationData(*operationData);
#endif

            LogicalLogRecord * logicalRecord = (*flushingRecords_)[i]->AsLogicalLogRecord();
            if (logicalRecord != nullptr && logicalRecord->IsLatencySensitiveRecord)
            {
                latencySensitiveRecords++;
            }

            serializationWatch.Stop();

            UpdatePerfCounter(PerfCounterName::AvgSerializationLatency, serializationWatch.ElapsedMilliseconds);

            flushWatch.Start();

            for (ULONG j = 0; j < operationData->BufferCount; j++)
            {
                numberOfBytes += (*operationData)[j]->QuerySize();

                status = co_await logicalLogStream_->AppendAsync(
                    *(*operationData)[j],
                    0,
                    (*operationData)[j]->QuerySize(),
                    CancellationToken());

                if (!NT_SUCCESS(status))
                {
                    goto FlushComplete;
                }
            }
            flushWatch.Stop();
        }

        flushWatch.Start();
        status = co_await logicalLogStream_->FlushWithMarkerAsync(CancellationToken());

        if (!NT_SUCCESS(status))
        {
            goto FlushComplete;
        }

        flushWatch.Stop();

        UpdateWriteStats(flushWatch, numberOfBytes);

        currentLogTailPosition_ += numberOfBytes;
        newTail = (*flushingRecords_)[flushingRecords_->Count() - 1];
        currentLogTailRecord_.Put(Ktl::Move(newTail));

        if (flushWatch.Elapsed > transactionalReplicatorConfig_->SlowLogIODuration)
        {
            // Update health tracker with slow IO
            ioMonitor_->OnSlowOperation();

            EventSource::Events->FlushEndWarning(
                TracePartitionId,
                ReplicaId,
                numberOfBytes,
                latencySensitiveRecords,
                flushWatch.ElapsedMilliseconds,
                serializationWatch.ElapsedMilliseconds,
                (double)writeSpeedBytesPerSecondSum_ / Constants::PhysicalLogWriterMovingAverageHistory,
                (double)runningLatencySumMs_ / Constants::PhysicalLogWriterMovingAverageHistory,
                logicalLogStream_->WritePosition - (LONG)numberOfBytes);
        }
        else
        {
            EventSource::Events->FlushEnd(
                TracePartitionId,
                ReplicaId,
                numberOfBytes,
                latencySensitiveRecords,
                flushWatch.ElapsedMilliseconds,
                serializationWatch.ElapsedMilliseconds,
                (double)writeSpeedBytesPerSecondSum_ / Constants::PhysicalLogWriterMovingAverageHistory,
                (double)runningLatencySumMs_ / Constants::PhysicalLogWriterMovingAverageHistory,
                logicalLogStream_->WritePosition - (LONG)numberOfBytes);
        }
    
    FlushComplete:
        if (!NT_SUCCESS(status))
        {
            loggingError_.store(status);

            LR_TRACE_EXCEPTION(
                L"FlushTask hit exception.",
                loggingError_.load());
        }

        // Disable group commit as there is no pending IO
        FlushCompleted();

        KSharedArray<AwaitableCompletionSource<void>::SPtr>::SPtr flushedTasks = flushingTasks;

        if (NT_SUCCESS(loggingError_.load()))
        {
            LoggedRecords::SPtr flushedRecords = LoggedRecords::Create(*flushingRecords_, GetThisAllocator());

            ProcessFlushedRecords(*flushedRecords);

            isFlushTask = ProcessFlushCompletion(*flushedRecords, *flushedTasks, flushingTasks);
        }
        else
        {
            LoggedRecords::CSPtr flushedRecords = LoggedRecords::Create(*flushingRecords_, loggingError_.load(), GetThisAllocator());

            ProcessFlushedRecords(*flushedRecords);

            isFlushTask = ProcessFlushCompletion(*flushedRecords, *flushedTasks, flushingTasks);
        }

        WakeupFlushWaiters(flushedTasks.RawPtr());

        if (isFlushTask && !NT_SUCCESS(loggingError_.load()))
        {
            FailedFlushTask(flushingTasks);
            isFlushTask = false;
        }

    } while (isFlushTask);

    co_return;
}

void PhysicalLogWriter::FailedFlushTask(__inout KSharedArray<AwaitableCompletionSource<void>::SPtr>::SPtr & flushingTasks)
{
    ASSERT_IF(
        NT_SUCCESS(loggingError_.load()),
        "{0}:FailedFlushTask | Logging error must be set",
        TraceId);

    bool isFlushTask = true;

    do
    {
        // It is important to process flushed records first before 
        // waking up flush waiters

        KSharedArray<AwaitableCompletionSource<void>::SPtr>::SPtr flushedTasks(flushingTasks);
        LoggedRecords::CSPtr flushedRecords = LoggedRecords::Create(*flushingRecords_, loggingError_.load(), GetThisAllocator());

        ProcessFlushedRecords(*flushedRecords);

        isFlushTask = ProcessFlushCompletion(
            *flushedRecords,
            *flushedTasks,
            flushingTasks);
        
        WakeupFlushWaiters(flushedTasks.RawPtr());
    }
    while (isFlushTask);
}

void PhysicalLogWriter::ProcessFlushedRecords(__in LoggedRecords const & loggedRecords)
{
    if (callbackManager_ != nullptr)
    {
        callbackManager_->ProcessFlushedRecords(loggedRecords);
    }
}

bool PhysicalLogWriter::ProcessFlushCompletion(
    __in LoggedRecords const & flushedRecords,
    __inout KSharedArray<AwaitableCompletionSource<void>::SPtr> & flushedTasks,
    __out KSharedArray<AwaitableCompletionSource<void>::SPtr>::SPtr & flushingTasks)
{
    bool isFlushTask = false;
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(flushLock_)
    {
        for (ULONG i = 0; i < flushedRecords.Count; i++)
        {
            LONG result = pendingFlushRecordsBytes_.fetch_sub(flushedRecords[i]->ApproximateSizeOnDisk);
            result = result - flushedRecords[i]->ApproximateSizeOnDisk;

#ifdef DBG
            ASSERT_IFNOT(
                result >= 0,
                "{0}: Subtraction of ApproximateSizeOnDisk {1} for record lsn: {2} yielded negative value for pending flush records {3}",
                TraceId,
                flushedRecords[i]->ApproximateSizeOnDisk,
                flushedRecords[i]->Lsn,
                result);
#endif
        }

        flushingRecords_ = pendingFlushRecords_;

        if (flushingRecords_ == nullptr)
        {
            if (pendingFlushTasks_ != nullptr)
            {
                for (ULONG i = 0; i < pendingFlushTasks_->Count(); i++)
                {
                    status = flushedTasks.Append((*pendingFlushTasks_)[i]);
                    THROW_ON_FAILURE(status);
                }

                pendingFlushTasks_ = nullptr;
            }

            flushingTasks = nullptr;
            isFlushTask = false;
        }
        else
        {
            flushingTasks = pendingFlushTasks_;
            pendingFlushRecords_ = nullptr;
            pendingFlushTasks_ = nullptr;

            isFlushTask = true;
        }
    }

    return isFlushTask;
}

void PhysicalLogWriter::SetTailRecord(__in LogRecord & tailRecord)
{
    currentLogTailPosition_ = tailRecord.RecordPosition + tailRecord.RecordSize;
    currentLogTailRecord_.Put(&tailRecord);
    currentLogTailPsn_ = tailRecord.Psn;

    PhysicalLogRecord * physicalLogRecord = tailRecord.AsPhysicalLogRecord();

    lastPhysicalRecord_ =
        physicalLogRecord == nullptr ?
        tailRecord.PreviousPhysicalRecord :
        physicalLogRecord;

    ASSERT_IFNOT(
        !LogRecord::IsInvalid(lastPhysicalRecord_.RawPtr()),
        "{0}:Invalid last physical record",
        TraceId);
}

OperationData::CSPtr PhysicalLogWriter::WriteRecord(
    __inout LogRecord & record, 
    __in ULONG bytesWritten)
{
    ASSERT_IFNOT(
        currentLogTailPosition_ + bytesWritten == (ULONG64)logicalLogStream_->GetWritePosition(),
        "{0}:Current log tail position: {1} + bytes written: {2} is not equal to logical log stream write position: {3}",
        TraceId,
        currentLogTailPosition_,
        bytesWritten,
        logicalLogStream_->GetWritePosition());

    ULONG64 recordPosition = currentLogTailPosition_ + bytesWritten;

    record.RecordPosition = recordPosition;

    return LogRecord::WriteRecord(
        record,
        recordWriter_,
        GetThisAllocator(),
        true,
        true,
        recomputeOffsets_);
}

void PhysicalLogWriter::PrepareToClose()
{
    closedError_.store(SF_STATUS_OBJECT_CLOSED);
}

Awaitable<NTSTATUS> PhysicalLogWriter::TruncateLogHeadAsync(__in ULONG64 headRecordPosition)
{
    ILogicalLog::SPtr logicalLog = logicalLogStream_;
    NTSTATUS status = STATUS_SUCCESS;

    if (logicalLog != nullptr)
    {
        ASSERT_IFNOT(
            headRecordPosition <= MAXULONG64,
            "{0}:Invalid head position during truncate log head",
            TraceId);

        Awaitable<NTSTATUS> truncateTask;

        // This is okay to do, because KTL and In-Memory log are synchronous
        // and FileLogicalLog takes its own async locks
        K_LOCK_BLOCK(flushLock_)
        {
            truncateTask = Ktl::Move(logicalLog->TruncateHead((LONG64)headRecordPosition));
        }

        co_await truncateTask;
    }

    co_return status;
}

Awaitable<NTSTATUS> PhysicalLogWriter::TruncateLogTail(__in LogRecord & newTailRecord)
{
    callbackManager_->FlushedPsn = newTailRecord.Psn + 1;
    SetTailRecord(newTailRecord);

    NTSTATUS status = co_await logicalLogStream_->TruncateTail(
        (LONG64)currentLogTailPosition_,
        CancellationToken());

    co_return status;
}

void PhysicalLogWriter::Test_ClearRecords()
{
    bufferedRecords_ = nullptr;
    pendingFlushRecords_ = nullptr;
    flushingRecords_ = nullptr;
}

void PhysicalLogWriter::InitializeMovingAverageKArray()
{
    for(ULONG i = 0; i < Constants::PhysicalLogWriterMovingAverageHistory; i++)
    {
        avgRunningLatencyMilliseconds_.Append(0);
        avgWriteSpeedBytesPerSecond_.Append(0);
    }
}

void PhysicalLogWriter::UpdateWriteStats(
    __in Common::Stopwatch watch,
    __in ULONG size) 
{
    ULONG bytesWritten = size;

    UpdatePerfCounter(PerfCounterName::LogFlushBytes, (LONG)bytesWritten);
    UpdatePerfCounter(PerfCounterName::AvgBytesPerFlush, (LONG)bytesWritten);

    LONG64 localAvgWriteSpeed = (LONG64)((bytesWritten * 10000 * 1000) / (ULONG)(watch.ElapsedTicks + 1));
    LONG64 localAvgRunningLatencyMs = watch.ElapsedMilliseconds;

    UpdatePerfCounter(PerfCounterName::AvgFlushLatency, localAvgRunningLatencyMs);

    runningLatencySumMs_ -= avgRunningLatencyMilliseconds_[0];
    avgRunningLatencyMilliseconds_.Remove(0);
    avgRunningLatencyMilliseconds_.Append(localAvgRunningLatencyMs);
    runningLatencySumMs_ += localAvgRunningLatencyMs;

    writeSpeedBytesPerSecondSum_ -= avgWriteSpeedBytesPerSecond_[0];
    avgWriteSpeedBytesPerSecond_.Remove(0);
    avgWriteSpeedBytesPerSecond_.Append(localAvgWriteSpeed);
    writeSpeedBytesPerSecondSum_ += localAvgWriteSpeed;
}

void PhysicalLogWriter::UpdatePerfCounter(
    __in PerfCounterName counterName,
    __in LONG64 value)
{
    if (perfCounters_)
    {
        switch (counterName)
        {
        case PerfCounterName::LogFlushBytes:
            perfCounters_->LogFlushBytes.IncrementBy(value);
            break;
        case PerfCounterName::AvgBytesPerFlush:
            perfCounters_->AvgBytesPerFlushBase.Increment();
            perfCounters_->AvgBytesPerFlush.IncrementBy(value);
            break;
        case PerfCounterName::AvgFlushLatency:
            perfCounters_->AvgFlushLatencyBase.Increment();
            perfCounters_->AvgFlushLatency.IncrementBy(value);
            break;
        case PerfCounterName::AvgSerializationLatency:
            perfCounters_->AvgSerializationLatencyBase.Increment();
            perfCounters_->AvgSerializationLatency.IncrementBy(value);
            break;
        default:
            break;
        }
    }
}
