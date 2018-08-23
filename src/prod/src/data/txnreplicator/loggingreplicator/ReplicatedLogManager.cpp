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

Common::StringLiteral const TraceComponent("ReplicatedLogManager");

ReplicatedLogManager::ReplicatedLogManager(
    __in PartitionedReplicaId const & traceId,
    __in LogManager & logManager,
    __in RecoveredOrCopiedCheckpointState & recoveredrCopiedCheckpointState,
    __in_opt IndexingLogRecord * const currentHead,
    __in RoleContextDrainState & roleContextDrainState,
    __in IStateReplicator & stateReplicator,
    __in InvalidLogRecords & invalidLogRecords)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , IReplicatedLogManager()
    , logManager_(&logManager)
    , roleContextDrainState_(&roleContextDrainState)
    , stateReplicator_(&stateReplicator)
    , recoveredOrCopiedCheckpointState_(&recoveredrCopiedCheckpointState)
    , invalidLogRecords_(&invalidLogRecords)
    , lsnOrderingLock_()
    , lsnOrderingQueue_(GetThisAllocator())
    , lsnOrderingTcs_()
    , operationAcceptedCount_(0)
    , appendCheckpointCallback_()
    , currentLogHeadRecord_(currentHead)
    , currentLogTailEpoch_()
    , currentLogTailLsn_(0)
    , lastCompletedEndCheckpointRecord_(nullptr)
    , lastIndexRecord_(nullptr)
    , lastInformationRecord_(nullptr)
    , lastInProgressCheckpointRecord_(nullptr)
    , lastInProgressTruncateHeadRecord_(nullptr)
    , lastLinkedPhysicalRecord_(nullptr)
    , progressVectorLock_()
    , progressVector_()
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"ReplicatedLogManager",
        reinterpret_cast<uintptr_t>(this));

    ASSERT_IFNOT(
        logManager_ != nullptr,
        "{0}: ReplicatedLogManager::Ctor | LogManager must not be null",
        TraceId);

    ASSERT_IFNOT(
        invalidLogRecords_ != nullptr,
        "{0}: ReplicatedLogManager::Ctor | InvalidLogRecords must not be null",
        TraceId);

    ASSERT_IFNOT(
        roleContextDrainState_ != nullptr,
        "{0}: ReplicatedLogManager::Ctor | RoleContextDrainState must not be null",
        TraceId);

    ASSERT_IFNOT(
        stateReplicator_ != nullptr,
        "{0}: ReplicatedLogManager::Ctor | IStateReplicator must not be null",
        TraceId);

    ASSERT_IFNOT(
        recoveredOrCopiedCheckpointState_ != nullptr,
        "{0}: ReplicatedLogManager::Ctor | RecoveredOrCopiedCheckpointState must not be null",
        TraceId);

    Reuse();
}

ReplicatedLogManager::~ReplicatedLogManager()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"ReplicatedLogManager",
        reinterpret_cast<uintptr_t>(this));
}

ReplicatedLogManager::SPtr ReplicatedLogManager::Create(
    __in PartitionedReplicaId const & traceId,
    __in LogManager & logManager,
    __in RecoveredOrCopiedCheckpointState & recoveredrCopiedCheckpointState,
    __in_opt IndexingLogRecord * const currentHead,
    __in RoleContextDrainState & roleContextDrainState,
    __in IStateReplicator & stateReplicator,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator)
{
    ReplicatedLogManager * pointer = _new(REPLICATEDLOGMANAGER_TAG, allocator) ReplicatedLogManager(
        traceId,
        logManager,
        recoveredrCopiedCheckpointState,
        currentHead,
        roleContextDrainState,
        stateReplicator,
        invalidLogRecords);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return ReplicatedLogManager::SPtr(pointer);
}

#ifdef DBG
void ReplicatedLogManager::ValidateOperationData(__in OperationData const & data)
{
    for (ULONG i = 0; i < data.BufferCount; i++)
    {
        ASSERT_IFNOT(
            data[i] != nullptr && data[i]->QuerySize() > 0,
            "ValidateOperationData | Operation data buffer cannot be non-null with size 0");
    }
}
#endif

Epoch ReplicatedLogManager::GetEpoch(
    __in FABRIC_SEQUENCE_NUMBER LSN) const
{
    Epoch result;

    K_LOCK_BLOCK(progressVectorLock_)
    {
        result = progressVector_->FindEpoch(LSN);
    }

    return result;
}

IndexingLogRecord::CSPtr ReplicatedLogManager::GetIndexingLogRecordForBackup() const
{
    // Threadsafe ptr.
    BeginCheckpointLogRecord::SPtr lastCompletedBeginCheckpointLogRecord = LastCompletedBeginCheckpointRecord;

    IndexingLogRecord::SPtr previousIndexingRecord = nullptr;
    PhysicalLogRecord::SPtr previousPhysicalRecord = LastCompletedBeginCheckpointRecord.RawPtr();

    BeginTransactionOperationLogRecord::SPtr earliestPendingBeginTransactionLogRecord = lastCompletedBeginCheckpointLogRecord->EarliestPendingTransaction;

    do
    {
        // Search back for the last indexing log record.
        do
        {
            PhysicalLogRecord::SPtr tmp = previousPhysicalRecord;
            previousPhysicalRecord = previousPhysicalRecord->PreviousPhysicalRecord;

            ASSERT_IFNOT(
                LogRecord::IsInvalid(previousPhysicalRecord.RawPtr()) == false,
                "{0}: Physical record before PSN {1} must exist.",
                TraceId,
                tmp->Psn);
        } while (previousPhysicalRecord->RecordType != LogRecordType::Indexing);

        previousIndexingRecord = dynamic_cast<IndexingLogRecord *>(previousPhysicalRecord.RawPtr());

        // Check to make sure that the indexing record is older that the earliest pending begin transaction record.
        // If there is no pending begin transaction record, we are done since we started scanning back from last completed begin checkpoint.
        if (earliestPendingBeginTransactionLogRecord == nullptr)
        {
            break;
        }

        // If indexing record is not old enough, keep scanning back.
        if (previousIndexingRecord->RecordPosition >= earliestPendingBeginTransactionLogRecord->RecordPosition)
        {
            continue;
        }

        break;
    } while (true);

    ASSERT_IFNOT(
        previousIndexingRecord != nullptr,
        "{0}: Previous Indexing Record to start the backup from must not be null",
        TraceId);

    return previousIndexingRecord.RawPtr();
}

NTSTATUS ReplicatedLogManager::AppendBarrier(
    __in BarrierLogRecord & barrierLogRecord,
    __in bool isPrimary)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (isPrimary)
    {
        LONG64 bufferedBytes = 0;
        status = ReplicateAndLog(barrierLogRecord, bufferedBytes, nullptr);
        return status;
    }

    AppendWithoutReplication(
        barrierLogRecord,
        nullptr);

    return status;
}

LONG64 ReplicatedLogManager::AppendWithoutReplication(
    __in LogicalLogRecord & record,
    __in_opt TransactionManager * const transactionManager)
{
    return Append(record, transactionManager, false);
}

Awaitable<void> ReplicatedLogManager::AwaitLsnOrderingTaskOnPrimaryAsync()
{
    // On the primary, since replication happens outside the lsnOrderingLock and logging happens within,
    // there could be pending replication operations that must be awaited to get logged before going ahead with
    // the close or change role or update epoch

    AwaitableCompletionSource<void>::SPtr lsnOrderingTask = nullptr;

    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        // if there is an existing wait tcs, return that
        if (lsnOrderingTcs_)
        {
            lsnOrderingTask = lsnOrderingTcs_;
        }
        else if (operationAcceptedCount_.load() > 0 || lsnOrderingQueue_.Count() > 0)
        {
            lsnOrderingTcs_ = CompletionTask::CreateAwaitableCompletionSource<void>(REPLICATEDLOGMANAGER_TAG, GetThisAllocator());
            lsnOrderingTask = lsnOrderingTcs_;
        }
    }

    if (lsnOrderingTask != nullptr)
    {
        co_await lsnOrderingTask->GetAwaitable();
    }

    co_return;
}

Awaitable<NTSTATUS> ReplicatedLogManager::AwaitReplication(__in LogicalLogRecord & logicalRecord)
{
    LogicalLogRecord::SPtr recordSPtr = &logicalRecord;
    NTSTATUS result = co_await recordSPtr->AwaitReplication();

    if (!NT_SUCCESS(result))
    {
        OnReplicationError(*recordSPtr, result);
    }

    co_return result;
}

void ReplicatedLogManager::CompleteCheckpoint()
{
    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        CompleteCheckPointLogRecord::SPtr record = CompleteCheckPointLogRecord::Create(
            currentLogTailLsn_,
            *currentLogHeadRecord_.Get(),
            lastLinkedPhysicalRecord_.Get().RawPtr(),
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            GetThisAllocator());

        lastLinkedPhysicalRecord_.Put(record.RawPtr());
        logManager_->PhysicalLogWriter->InsertBufferedRecord(*record);
    }
}

void ReplicatedLogManager::EndCheckpoint(__in BeginCheckpointLogRecord & lastInProgressCheckpointRecord)
{
    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        EndCheckpointCallerHoldsLock(lastInProgressCheckpointRecord);
    }
}

Awaitable<void> ReplicatedLogManager::FlushEndCheckpointAsync()
{
    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        EndCheckpointCallerHoldsLock(*LastInProgressCheckpointRecord);
    }
    co_await logManager_->FlushAsync(L"FlushEndCheckpointAsync");
    co_return;
}

Awaitable<void> ReplicatedLogManager::FlushInformationRecordAsync(
    __in InformationEvent::Enum type,
    __in bool closeLog,
    __in KStringView const initiator)
{
    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        InformationCallerHoldsLock(type);
        if (closeLog)
        {
            logManager_->PrepareToClose();
        }
    }

    co_await logManager_->FlushAsync(initiator);
    co_return;
}

LogRecord::SPtr ReplicatedLogManager::GetEarliestRecord()
{
    LogRecord::SPtr record;

    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        record = GetEarliestRecordCallerHoldsLock();
    }

    return record;
}

void ReplicatedLogManager::Index()
{
    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        IndexingLogRecord::SPtr record = IndexingLogRecord::Create(
            currentLogTailEpoch_,
            currentLogTailLsn_,
            lastLinkedPhysicalRecord_.Get().RawPtr(),
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            GetThisAllocator());

        lastIndexRecord_.Put(record.RawPtr());

        logManager_->PhysicalLogWriter->InsertBufferedRecord(*record);
    }
}

void ReplicatedLogManager::InformationCallerHoldsLock(__in InformationEvent::Enum type)
{
    InformationLogRecord::SPtr record = InformationLogRecord::Create(
        currentLogTailLsn_,
        lastLinkedPhysicalRecord_.Get().RawPtr(),
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        type,
        GetThisAllocator());

    lastInformationRecord_.Put(record.RawPtr());

    logManager_->PhysicalLogWriter->InsertBufferedRecord(*record);
}

void ReplicatedLogManager::Information(__in InformationEvent::Enum type)
{
    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        InformationCallerHoldsLock(type);
    }
}

void ReplicatedLogManager::OnCompletePendingCheckpoint()
{
    lastInProgressCheckpointRecord_.Put(nullptr);
}

void ReplicatedLogManager::OnCompletePendingLogHeadTruncation()
{
    lastInProgressTruncateHeadRecord_.Put(nullptr);
}

void ReplicatedLogManager::OnTruncateTailOfLastLinkedPhysicalRecord()
{
    PhysicalLogRecord::SPtr record = lastLinkedPhysicalRecord_.Get()->LinkedPhysicalRecord;
    lastLinkedPhysicalRecord_.Put(Ktl::Move(record));
}

NTSTATUS ReplicatedLogManager::ReplicateAndLog(
    __in LogicalLogRecord & record,
    __out LONG64 & bufferedRecordBytes,
    __in_opt TransactionManager * const transactionManager)
{
    bufferedRecordBytes = 0;
    NTSTATUS status = BeginReplicate(record);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    bufferedRecordBytes = Append(record, transactionManager, true);
    return status;
}

void ReplicatedLogManager::Reuse()
{
    progressVector_ = nullptr;
    lastLinkedPhysicalRecord_.Put(nullptr);
    lastInformationRecord_.Put(invalidLogRecords_->Inv_InformationLogRecord.RawPtr());
    currentLogTailEpoch_ = Epoch::InvalidEpoch();
    currentLogTailLsn_ = Constants::InvalidLsn;
    lsnOrderingTcs_ = nullptr;
    operationAcceptedCount_.store(0);
    lsnOrderingQueue_.Clear();
}

void ReplicatedLogManager::Reuse(
    __in ProgressVector & progressVector,
    __in_opt EndCheckpointLogRecord * const lastCompletedEndCheckpointRecord,
    __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
    __in_opt InformationLogRecord * const lastInformationRecord,
    __in IndexingLogRecord & currentLogHeadRecord,
    __in Epoch const & tailEpoch,
    __in LONG64 tailLsn)
{
    lastInProgressCheckpointRecord_.Put(nullptr);
    lastInProgressTruncateHeadRecord_.Put(nullptr);
    progressVector_ = &progressVector;
    lastCompletedEndCheckpointRecord_.Put(lastCompletedEndCheckpointRecord);
    lastLinkedPhysicalRecord_.Put(lastLinkedPhysicalRecord);
    currentLogHeadRecord_.Put(&currentLogHeadRecord);
    lastInformationRecord_.Put(lastInformationRecord);
    currentLogTailEpoch_ = tailEpoch;
    currentLogTailLsn_ = tailLsn;
}

void ReplicatedLogManager::SetCheckpointCallback(__in AppendCheckpointCallback const & appendCheckpointCallback)
{
    appendCheckpointCallback_ = appendCheckpointCallback;
}

void ReplicatedLogManager::SetTailEpoch(__in Epoch const & epoch)
{
    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        currentLogTailEpoch_ = epoch;
    }
}

void ReplicatedLogManager::SetTailLsn(__in LONG64 lsn)
{
    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        currentLogTailLsn_ = lsn;
    }
}

void ReplicatedLogManager::InsertBeginCheckpoint(__in BeginCheckpointLogRecord & record)
{
    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        BeginCheckpointCallerHoldsLock(record);
    }
}

void ReplicatedLogManager::InsertTruncateHeadCallerHoldsLock(
    __in TruncateHeadLogRecord & record,
    __in IndexingLogRecord & headRecord)
{
    lastLinkedPhysicalRecord_.Put(&record);
    currentLogHeadRecord_.Put(&headRecord);

    lastInProgressTruncateHeadRecord_.Put(&record);
    logManager_->PhysicalLogWriter->InsertBufferedRecord(record);
}

void ReplicatedLogManager::OnReplicationError(
    __in LogicalLogRecord const & record,
    __in NTSTATUS errorCode)
{
    bool isExpected =
        errorCode == SF_STATUS_NOT_PRIMARY ||
        errorCode == SF_STATUS_RECONFIGURATION_PENDING ||
        errorCode == SF_STATUS_REPLICATION_QUEUE_FULL ||
        errorCode == SF_STATUS_NO_WRITE_QUORUM ||
        errorCode == SF_STATUS_OBJECT_CLOSED ||
        errorCode == SF_STATUS_REPLICATION_OPERATION_TOO_LARGE ||
        errorCode == STATUS_CANCELLED;

    if (isExpected)
    {
        LR_TRACE_EXCEPTIONINFORECORD(
            L"ProcessReplicationException",
            record,
            errorCode);
        // do nothing
    }
    else if (!roleContextDrainState_->IsClosing)
    {
        LR_TRACE_UNEXPECTEDEXCEPTIONRECORD(
            L"ProcessReplicationException",
            record,
            errorCode);

        roleContextDrainState_->ReportFault();

#ifdef DBG
        Common::Assert::CodingError(
            "{0}: Unexpected status from replicate: {1}",
            TraceId,
            errorCode);
#endif
    }
}

TruncateHeadLogRecord::SPtr ReplicatedLogManager::TruncateHead(
    __in bool isStable,
    __in LONG64 lastPeriodicTruncationTimeStampTicks,
    __in IsGoodLogHeadCandidateCalculator & isGoodLogHeadCandidateCalculator)
{
    TruncateHeadLogRecord::SPtr record = nullptr;

    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        LogRecord::SPtr earliestRecord = GetEarliestRecordCallerHoldsLock();

        IndexingLogRecord::SPtr previousIndexingRecord = nullptr;
        PhysicalLogRecord::SPtr previousPhysicalRecord = LastCompletedBeginCheckpointRecord.RawPtr();

        IndexingLogRecord::SPtr currentLogHead = currentLogHeadRecord_.Get();

        do
        {
            // Search for the last Indexing log Record 
            do
            {
                previousPhysicalRecord = previousPhysicalRecord->PreviousPhysicalRecord;
            } while (previousPhysicalRecord->RecordType != LogRecordType::Enum::Indexing);

            previousIndexingRecord = dynamic_cast<IndexingLogRecord *>(previousPhysicalRecord.RawPtr());
            ASSERT_IF(
                previousIndexingRecord == nullptr,
                "{0}:Unexpected dynamic cast failure",
                TraceId);

            // This index record is not before the earliest pending transaction record 
            if (previousIndexingRecord->RecordPosition >= earliestRecord->RecordPosition)
            {
                continue;
            }

            // We reached log head, so do not continue to look for last index log record
            if (previousIndexingRecord.RawPtr() == currentLogHead.RawPtr())
            {
                return nullptr;
            }

            if (isGoodLogHeadCandidateCalculator(*previousIndexingRecord))
            {
                break;
            }
        } while (true);

        // Only truncation operations initiated by configured interval record timestamp
        record = TruncateHeadLogRecord::Create(
            *previousIndexingRecord,
            currentLogTailLsn_,
            lastLinkedPhysicalRecord_.Get().RawPtr(),
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            isStable,
            lastPeriodicTruncationTimeStampTicks,
            GetThisAllocator());

        InsertTruncateHeadCallerHoldsLock(*record, *previousIndexingRecord);
    }

    return record;
}

void ReplicatedLogManager::TruncateTail(__in LONG64 tailLsn)
{
    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        TruncateTailLogRecord::SPtr record = TruncateTailLogRecord::Create(
            tailLsn,
            *lastLinkedPhysicalRecord_.Get(),
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            GetThisAllocator());

        logManager_->PhysicalLogWriter->InsertBufferedRecord(*record);
    }
}

void ReplicatedLogManager::UpdateEpochRecord(
    __in UpdateEpochLogRecord & record)
{
    UpdateEpochLogRecord::SPtr recordSPtr = &record;

    ASSERT_IFNOT(
        currentLogTailLsn_ == recordSPtr->Lsn,
        "{0}:Current log tail lsn:{1} is not same as record lsn: {2} during update epoch log record",
        TraceId,
        currentLogTailLsn_,
        recordSPtr->Lsn);

    EventSource::Events->UpdateEpochRecord(
        TracePartitionId,
        ReplicaId,
        recordSPtr->EpochValue.DataLossVersion,
        recordSPtr->EpochValue.ConfigurationVersion,
        recordSPtr->Lsn,
        static_cast<ULONG>(roleContextDrainState_->ReplicaRole));

    AppendWithoutReplication(
        *recordSPtr,
        nullptr);

    K_LOCK_BLOCK(progressVectorLock_)
    {
        progressVector_->Append(ProgressVectorEntry(*recordSPtr));
    }

    SetTailEpoch(record.EpochValue);
}

void ReplicatedLogManager::AddCheckpointIfNeededCallerHoldsLock(__in bool isPrimary)
{
    // Ensure that the checkpoint callback is initialized
    ASSERT_IFNOT(
        (BOOLEAN)(appendCheckpointCallback_) == TRUE,
        "{0}:appendCheckpointCallback_ must be true",
        TraceId);

    BeginCheckpointLogRecord::SPtr record = appendCheckpointCallback_(isPrimary);

    if (record == nullptr)
    {
        return;
    }

    BeginCheckpointCallerHoldsLock(*record);
}

LONG64 ReplicatedLogManager::Append(
    __in LogicalLogRecord & record,
    __in_opt TransactionManager * const transactionManager,
    __in bool isPrimary)
{
    LONG64 bufferedBytes = 0;
    TransactionLogRecord::SPtr txLogRecord = nullptr;
    AwaitableCompletionSource<void>::SPtr lsnOrderingTcsToSignal = nullptr;

    if (LogRecord::IsTransactionLogRecord(record))
    {
        ASSERT_IFNOT(
            transactionManager != nullptr,
            "{0}:Null transaction manager during append log record",
            TraceId);

        LogicalLogRecord::SPtr recordSPtr = &record;
        txLogRecord = dynamic_cast<TransactionLogRecord *>(recordSPtr.RawPtr());
 
        ASSERT_IF(
            txLogRecord == nullptr,
            "{0}: Unexpected dynamic cast failure",
            TraceId);

        transactionManager->CreateTransactionState(currentLogTailEpoch_, *txLogRecord);
    }

    K_LOCK_BLOCK(lsnOrderingLock_)
    {
        bufferedBytes = LsnOrderingInsertCallerHoldsLock(
            record,
            isPrimary,
            lsnOrderingTcsToSignal);
    }

    if (lsnOrderingTcsToSignal != nullptr)
    {
        // Do this outside the lock above and also after setting the this.lsnorderingtcs to NULL as this set result can end up invoking a completion to run on the same thread,
        // which ends up invoking another call to Append. This happened in a unit test on the UpdateEpoch code path on primary
        bool result = lsnOrderingTcsToSignal->TrySet();

        ASSERT_IFNOT(
            result,
            "{0}:Could not set lsn ordering Tcs in append log record",
            TraceId);
    }

    return bufferedBytes;
}

void ReplicatedLogManager::BeginCheckpointCallerHoldsLock(__in BeginCheckpointLogRecord & record)
{
    ASSERT_IFNOT(
        lastInProgressCheckpointRecord_.Get() == nullptr,
        "{0}:BeginCheckpointCallerHoldsLock : last in progress checkpoint record must be null before BeginCheckpoint",
        TraceId);

    lastInProgressCheckpointRecord_.Put(&record);
    logManager_->PhysicalLogWriter->InsertBufferedRecord(record);
}

void ReplicatedLogManager::EndCheckpointCallerHoldsLock(__in BeginCheckpointLogRecord & lastInProgressCheckpointRecord)
{
    EndCheckpointLogRecord::SPtr record = EndCheckpointLogRecord::Create(
        lastInProgressCheckpointRecord,
        *currentLogHeadRecord_.Get(),
        currentLogTailLsn_,
        lastLinkedPhysicalRecord_.Get().RawPtr(),
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());

    lastCompletedEndCheckpointRecord_.Put(record.RawPtr());
    lastLinkedPhysicalRecord_.Put(record.RawPtr());
    logManager_->PhysicalLogWriter->InsertBufferedRecord(*record);
}

LogRecord::SPtr ReplicatedLogManager::GetEarliestRecordCallerHoldsLock()
{
    LogRecord::SPtr earliestRecord = LastCompletedBeginCheckpointRecord.RawPtr();

    if (LastCompletedBeginCheckpointRecord->EarliestPendingTransaction != nullptr)
    {
        earliestRecord = LastCompletedBeginCheckpointRecord->EarliestPendingTransaction.RawPtr();
    }

    ASSERT_IF(
        earliestRecord == nullptr,
        "{0}: EarliestRecord should not be null",
        TraceId);

    return earliestRecord;
}

LONG64 ReplicatedLogManager::InsertBufferedRecordCallerHoldsLock(
    __in LogicalLogRecord & record,
    __in bool isPrimary,
    __out LONG & pendingCount)
{
    LONG64 result = logManager_->PhysicalLogWriter->InsertBufferedRecord(record);
    UpdateEpochLogRecord::SPtr updateEpochRecord = nullptr;
    LogicalLogRecord::SPtr recordSPtr = &record;

    if (record.RecordType == LogRecordType::Enum::UpdateEpoch)
    {
        updateEpochRecord = dynamic_cast<UpdateEpochLogRecord *>(recordSPtr.RawPtr());
        ASSERT_IF(
            updateEpochRecord == nullptr,
            "{0}:Unexpected dynamic cast failure",
            TraceId);
    }

    ASSERT_IFNOT(
        (currentLogTailLsn_ + 1) == record.Lsn || (updateEpochRecord != nullptr && updateEpochRecord->Lsn == currentLogTailLsn_),
        "{0}:Invalid current log tail lsn: {1} during inserting buffered record with lsn: {2}",
        TraceId,
        currentLogTailLsn_,
        record.Lsn);

    if (updateEpochRecord == nullptr)
    {
        // Increment tail lsn only on appending a logical record that is not an update epoch
        ++currentLogTailLsn_;
    }

    if (isPrimary)
    {
        pendingCount = OnOperationLogInitiationCallerHoldsLock();
    }

    if (record.RecordType == LogRecordType::Enum::Barrier)
    {
        BarrierLogRecord::SPtr barrierRecord = dynamic_cast<BarrierLogRecord *>(recordSPtr.RawPtr()); 
        ASSERT_IF(
            barrierRecord == nullptr,
            "{0}:Unexpected dynamic cast failure",
            TraceId);

        OnBarrierBufferedCallerHoldsLock(*barrierRecord, isPrimary);
    }

    return result;
}

LONG64 ReplicatedLogManager::LsnOrderingInsertCallerHoldsLock(
    __in LogicalLogRecord & record,
    __in bool isPrimary,
    __out AwaitableCompletionSource<void>::SPtr & lsnOrderingTcsToSignal)
{
    LONG64 recordLsn = record.Lsn;
    LONG64 result = 0;
    LONG pendingCount = -1;

    // First every update epoch record on full copy secondary has UpdateEpoch(0,0)
    ASSERT_IFNOT(
        recordLsn > Constants::ZeroLsn || record.RecordType == LogRecordType::Enum::UpdateEpoch,
        "{0}:Invalid log record lsn: {1} of type: {2} during lsn ordering",
        TraceId,
        record.Lsn,
        record.RecordType);

    if (recordLsn > (currentLogTailLsn_ + 1))
    {
        LONG i = 0;
        for (i = lsnOrderingQueue_.Count() - 1; i >= 0; i--)
        {
            if (recordLsn > lsnOrderingQueue_[i]->Lsn)
            {
                lsnOrderingQueue_.InsertAt(i + 1, &record);
                break;
            }
        }

        if (i == -1)
        {
            lsnOrderingQueue_.InsertAt(0, &record);
        }

        return -1;
    }

    result = InsertBufferedRecordCallerHoldsLock(record, isPrimary, pendingCount);

    ULONG j = 0;
    for (j = 0; j < lsnOrderingQueue_.Count(); j++)
    {
        if (lsnOrderingQueue_[j]->Lsn > (currentLogTailLsn_ + 1))
        {
            break;
        }

        InsertBufferedRecordCallerHoldsLock(*lsnOrderingQueue_[j], isPrimary, pendingCount);
    }
    
    if (j > 0)
    {
        BOOLEAN removedResult = lsnOrderingQueue_.RemoveRange(0, j);
        ASSERT_IFNOT(removedResult == TRUE, "Could not remove from lsn ordering queue");
    }

    if (lsnOrderingTcs_ != nullptr &&
        pendingCount == 0 &&
        lsnOrderingQueue_.Count() == 0)
    {
        lsnOrderingTcsToSignal = lsnOrderingTcs_;
        lsnOrderingTcs_.Reset();
    }

    return result;
}

void ReplicatedLogManager::OnBarrierBufferedCallerHoldsLock(
    __in BarrierLogRecord & record,
    __in bool isPrimary)
{
    EventSource::Events->AcceptBarrier(
        TracePartitionId,
        ReplicaId,
        record.Lsn);

    AddCheckpointIfNeededCallerHoldsLock(isPrimary);

// ToTask is invoked as we do not await on this call
    Awaitable<void> flushAsync = logManager_->FlushAsync(L"OnBarrierBufferedCallerHoldsLock");
    ToTask(flushAsync);
}

void ReplicatedLogManager::OnOperationAcceptance()
{
    ++operationAcceptedCount_;
}

LONG ReplicatedLogManager::OnOperationLogInitiationCallerHoldsLock()
{
    LONG pendingCount = --operationAcceptedCount_;
    ASSERT_IFNOT(
        pendingCount >= 0,
        "{0}:Pending op accepted count is zero during log initiation",
        TraceId);

    return pendingCount;
}

void ReplicatedLogManager::OnOperationAcceptanceException()
{
    LONG pendingCount = --operationAcceptedCount_;
    ASSERT_IFNOT(
        pendingCount >= 0,
        "{0}:Pending op accepted count is zero",
        TraceId);

    AwaitableCompletionSource<void>::SPtr localTcs = nullptr;

    if (pendingCount == 0)
    {
        K_LOCK_BLOCK(lsnOrderingLock_)
        {
            if (lsnOrderingQueue_.Count() == 0 &&
                lsnOrderingTcs_ != nullptr)
            {
                localTcs = lsnOrderingTcs_;
                lsnOrderingTcs_ = nullptr;
            }
        }
    }

    if (localTcs != nullptr)
    {
        // Do this outside the lock above and also after setting the this.lsnorderingtcs to NULL as this set result can end up invoking a completion to run on the same thread,
        // which ends up invoking another call to Append. This happened in a unit test on the UpdateEpoch code path on primary
        bool result = localTcs->TrySet();
        ASSERT_IFNOT(
            result,
            "{0}:Could not set local Tcs in operation acceptance exception processing",
            TraceId);
    }
}

NTSTATUS ReplicatedLogManager::BeginReplicate(__in LogicalLogRecord & record)
{
    ASSERT_IFNOT(
        record.Lsn <= Constants::ZeroLsn && record.RecordType != LogRecordType::Enum::UpdateEpoch,
        "{0}:Invalid log record lsn: {1} of type: {2} during replicate",
        TraceId,
        record.Lsn,
        record.RecordType);
    
    if (roleContextDrainState_->IsClosing)
    {
        return SF_STATUS_OBJECT_CLOSED;
    }
    else if (roleContextDrainState_->ReplicaRole != FABRIC_REPLICA_ROLE_PRIMARY)
    {
        return SF_STATUS_NOT_PRIMARY;
    }

    OperationData::CSPtr operationData = record.SerializeLogicalData();

#ifdef DBG
    ValidateOperationData(*operationData);
#endif

    LONG64 sequenceNumber = Constants::ZeroLsn;

    OnOperationAcceptance();
    NTSTATUS status = STATUS_SUCCESS;

    CompletionTask::SPtr replicationTask = stateReplicator_->ReplicateAsync(
        *operationData,
        sequenceNumber);
    
    // Return insufficient resources if this is null
    if (!replicationTask)
    {
        OnReplicationError(record, STATUS_INSUFFICIENT_RESOURCES);
        OnOperationAcceptanceException();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    record.SetReplicationTask(Ktl::Move(replicationTask));
    record.Lsn = sequenceNumber;

    if (sequenceNumber <= Constants::ZeroLsn)
    {
        ASSERT_IFNOT(
            record.IsReplicated,
            "{0}:Invalid replication task exception completion state",
            TraceId);

        status = record.ReplicationResult;

        ASSERT_IF(
            NT_SUCCESS(status),
            "{0}: BeginReplicate expected to fail when lsn is invalid",
            TraceId);

        OnReplicationError(record, status);
        OnOperationAcceptanceException();
    }

    return status;
}
