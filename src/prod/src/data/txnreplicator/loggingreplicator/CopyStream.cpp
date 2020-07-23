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

CopyStream::CopyStream(
    __in PartitionedReplicaId const & traceId,
    __in ReplicatedLogManager & replicatedLogManager,
    __in IStateProviderManager & stateManager,
    __in ICheckpointManager & checkpointManager,
    __in WaitForLogFlushUpToLsnCallback const & waitForLogFlushUptoLsn,
    __in LONG64 replicaId,
    __in LONG64 uptoLsn,
    __in OperationDataStream & copyContext,
    __in CopyStageBuffers & copyStageBuffers,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in bool hasPersistedState,
    __in KAllocator & allocator)
    : OperationDataStream()
    , PartitionedReplicaTraceComponent(traceId)
    , replicatedLogManager_(&replicatedLogManager)
    , checkpointManager_(&checkpointManager)
    , stateManager_(&stateManager)
    , replicaId_(replicaId)
    , copyStageBuffers_(&copyStageBuffers)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , copyContext_(&copyContext)
    , waitForLogFlushUptoLsn_(waitForLogFlushUptoLsn)
    , beginCheckpointRecord_(nullptr)
    , copiedRecordNumber_(0)
    , copyStage_(CopyStage::Enum::CopyMetadata)
    , copyStateStream_(nullptr)
    , currentTargetLsn_(Constants::InvalidLsn)
    , latestRecoveredAtomicRedoOperationLsn_(Constants::InvalidLsn)
    , logRecordsToCopy_(nullptr)
    , sourceStartingLsn_(Constants::InvalidLsn)
    , targetLogHeadEpoch_(Epoch::InvalidEpoch())
    , targetLogHeadLsn_(Constants::InvalidLsn)
    , targetProgressVector_()
    , targetReplicaId_(0)
    , targetStartingLsn_(Constants::InvalidLsn)
    , uptoLsn_(uptoLsn)
    , bw_(allocator)
    , hasPersistedState_(hasPersistedState)
#ifdef DBG
    , previousGetNextCallReturnedTime_(Common::Stopwatch::Now())
#endif
    , isDisposed_(false)
    , maxReplicationMessageSizeInBytes_(replicatedLogManager_->StateReplicator->GetMaxReplicationMessageSize() * 1024 * 1024)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"CopyStream",
        reinterpret_cast<uintptr_t>(this));
}

CopyStream::~CopyStream()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"CopyStream",
        reinterpret_cast<uintptr_t>(this));

    ASSERT_IFNOT(
        isDisposed_,
        "{0}: copy stream is not disposed.",
        TraceId);
}

CopyStream::SPtr CopyStream::Create(
    __in PartitionedReplicaId const & traceId,
    __in ReplicatedLogManager & replicatedLogManager,
    __in IStateProviderManager & stateManager,
    __in ICheckpointManager & checkpointManager,
    __in WaitForLogFlushUpToLsnCallback const & waitForLogFlushUptoLsn,
    __in LONG64 replicaId,
    __in LONG64 uptoLsn,
    __in OperationDataStream & copyContext,
    __in CopyStageBuffers & copyStageBuffers,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in bool hasPersistedState,
    __in KAllocator & allocator)
{
    CopyStream * pointer = _new(COPYSTREAM_TAG, allocator) CopyStream(
        traceId, 
        replicatedLogManager, 
        stateManager, 
        checkpointManager, 
        waitForLogFlushUptoLsn, 
        replicaId, 
        uptoLsn, 
        copyContext, 
        copyStageBuffers, 
        transactionalReplicatorConfig,
        hasPersistedState,
        allocator);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return CopyStream::SPtr(pointer);
}

void CopyStream::Dispose()
{
    if (isDisposed_)
    {
        return;
    }
    
    if (logRecordsToCopy_ != nullptr)
    {
        logRecordsToCopy_->Dispose();
    }

    if (copyStateStream_ != nullptr)
    {
        copyStateStream_->Dispose();
    }

    isDisposed_ = true;
}

Awaitable<NTSTATUS> CopyStream::GetNextAsync(
    __in CancellationToken const & cancellationToken,
    OperationData::CSPtr & result) noexcept
{
    NTSTATUS status = co_await GetNextAsyncSafe(cancellationToken, result);
#ifdef DBG
    previousGetNextCallReturnedTime_ = Common::Stopwatch::Now();
#endif

    if (!NT_SUCCESS(status))
    {
        if (logRecordsToCopy_ != nullptr)
        {
            logRecordsToCopy_->Dispose();
        }
    }

    co_return status;
}

Awaitable<NTSTATUS> CopyStream::GetNextAsyncSafe(
    __in CancellationToken const & cancellationToken,
    OperationData::CSPtr & result)
{
    Common::TimeSpan apiCallLatency;
    NTSTATUS status = STATUS_SUCCESS;
    OperationData::CSPtr data;

#ifdef DBG
    apiCallLatency = Common::Stopwatch::Now() - previousGetNextCallReturnedTime_;
#endif

    if (copyStage_ == CopyStage::Enum::CopyMetadata)
    {
        status = co_await waitForLogFlushUptoLsn_(uptoLsn_);
        CO_RETURN_ON_FAILURE(status);

        targetProgressVector_ = ProgressVector::Create(this->GetThisAllocator());
        Epoch lastTargetProgressVectorEntryEpoch;

        status = co_await copyContext_->GetNextAsync(cancellationToken, data);

        CO_RETURN_ON_FAILURE(status);

        if (data != nullptr)
        {
            ASSERT_IF(
                data->get_BufferCount() != 1,
                "{0}: data buffer count != 1. count is {1}",
                TraceId,
                data->get_BufferCount());

            KBuffer::CSPtr buffer = (*data)[0];
            BinaryReader br(*buffer, GetThisAllocator());

            br.Read(targetReplicaId_);
            targetProgressVector_ = ProgressVector::Create(GetThisAllocator());
            targetProgressVector_->Read(br, false);
            LONG64 targetLogHeadDataLossNumber;
            LONG64 targetLogHeadConfigurationNumber;
            br.Read(targetLogHeadDataLossNumber);
            br.Read(targetLogHeadConfigurationNumber);
            targetLogHeadEpoch_ = Epoch(targetLogHeadDataLossNumber, targetLogHeadConfigurationNumber);
            br.Read(targetLogHeadLsn_);
            br.Read(currentTargetLsn_);
            br.Read(latestRecoveredAtomicRedoOperationLsn_);
        }
        else
        {
            ASSERT_IFNOT(hasPersistedState_ == false, "{0}: Copy context should not be null if hasPersistedState=true", TraceId);

            // Since copy context is null, we don't know at this level which replica we are copying to, so we will use an arbitrary id.
            // The replica id is used for tracing, and thus doesn't affect correctness
            targetReplicaId_ = 999;

            // Because state is not persisted, we know that the target replica is starting empty so the entire log has to be copied
            targetLogHeadEpoch_ = Epoch(0, 0);

            ProgressVectorEntry pe(targetLogHeadEpoch_, 1, 1, Common::DateTime::Now());
            targetProgressVector_->Insert(pe);

            targetLogHeadLsn_ = 1;
            currentTargetLsn_ = 1;
        }

        lastTargetProgressVectorEntryEpoch = targetProgressVector_->LastProgressVectorEntry.CurrentEpoch;
        Epoch currentLogHeadRecordEpoch = replicatedLogManager_->CurrentLogHeadRecord->CurrentEpoch;
        Epoch currentLogTailRecordEpoch = replicatedLogManager_->CurrentLogTailEpoch;

        wstring targetMessage = Common::wformatString("\r\nTarget Replica Copy Context: {0}\r\n TargetLogHeadEpoch: <{1}:{2:x}>\r\n TargetLogHeadLsn: {3}\r\n TargetLogTailEpoch: <{4}:{5:x}>\r\n TargetLogTailLsn: {6}\r\n",
            targetReplicaId_,
            targetLogHeadEpoch_.DataLossVersion,
            targetLogHeadEpoch_.ConfigurationVersion,
            targetLogHeadLsn_,
            lastTargetProgressVectorEntryEpoch.DataLossVersion,
            lastTargetProgressVectorEntryEpoch.ConfigurationVersion,
            currentTargetLsn_);

        wstring sourceMessage = Common::wformatString("\r\nSourceLogHeadEpoch: <{0}:{1:x}> \r\nSourceLogHeadLsn: {2} \r\nSourceLogTailEpoch: <{3}:{4:x}> \r\nSourceLogTailLsn: {5}\r\n\r\nTarget ProgressVector: {6}\r\n\r\nSource ProgressVector: {7} \r\nLastRecoveredAtomicRedoOperationLsn: {8}\r\n HasPersistedState: {9}\r\n",
            currentLogHeadRecordEpoch.DataLossVersion,
            currentLogHeadRecordEpoch.ConfigurationVersion,
            replicatedLogManager_->CurrentLogHeadRecord->Lsn,
            currentLogTailRecordEpoch.DataLossVersion,
            currentLogTailRecordEpoch.ConfigurationVersion,
            replicatedLogManager_->CurrentLogTailLsn,
            targetProgressVector_->ToString(Constants::ProgressVectorMaxStringSizeInKb / 2),
            replicatedLogManager_->ProgressVectorValue->ToString(Constants::ProgressVectorMaxStringSizeInKb / 2),
            latestRecoveredAtomicRedoOperationLsn_,
            hasPersistedState_);

        EventSource::Events->CopyStream(
            TracePartitionId,
            ReplicaId,
            targetMessage,
            sourceMessage);

        bool lockAcquired = co_await checkpointManager_->AcquireBackupAndCopyConsistencyLockAsync(L"Copy", KDuration(MAXLONGLONG), CancellationToken::None);

        ASSERT_IFNOT(
            lockAcquired,
            "{0}: checkpoint manager lock could not be acquired in copystream",
            TraceId);

        CopyModeFlag::Enum copyMode = checkpointManager_->GetLogRecordsToCopy(
                *targetProgressVector_,
                targetLogHeadEpoch_,
                targetLogHeadLsn_,
                currentTargetLsn_,
                latestRecoveredAtomicRedoOperationLsn_,
                targetReplicaId_,
                sourceStartingLsn_,
                targetStartingLsn_,
                logRecordsToCopy_,
                beginCheckpointRecord_);

        checkpointManager_->ReleaseBackupAndCopyConsistencyLock(L"Copy");

        CopyStage::Enum copyStageToWrite;
        if (copyMode == CopyModeFlag::Enum::None)
        {
            copyStage_ = CopyStage::Enum::CopyNone;
            copyStageToWrite = CopyStage::Enum::CopyNone;
        }
        else if (copyMode == CopyModeFlag::Enum::Full)
        {
            copyStage_ = CopyStage::Enum::CopyState;
            copyStageToWrite = CopyStage::Enum::CopyState;
            status = stateManager_->GetCurrentState(targetReplicaId_, copyStateStream_);

            CO_RETURN_ON_FAILURE(status);
        }
        else if ((copyMode & CopyModeFlag::Enum::FalseProgress) != 0)
        {
            ASSERT_IFNOT(
                sourceStartingLsn_ <= targetStartingLsn_,
                "{0}: sourceStartingLsn ({1}) < targetStartingLsn ({2})",
                TraceId,
                sourceStartingLsn_,
                targetStartingLsn_);

            copyStage_ = CopyStage::Enum::CopyFalseProgress;
            copyStageToWrite = CopyStage::Enum::CopyFalseProgress;
        }
        else
        {
            copyStage_ = CopyStage::Enum::CopyScanToStartingLsn;
            copyStageToWrite = CopyStage::Enum::CopyLog;
        }

        result = GetCopyMetadata(copyStageToWrite);
        co_return status;
    }

    if (copyStage_ == CopyStage::Enum::CopyState)
    {
        status = co_await copyStateStream_->GetNextAsync(cancellationToken, data);

        CO_RETURN_ON_FAILURE(status);

        if (data != nullptr)
        {
            OperationData::SPtr dataCast = const_cast<OperationData *>(data.RawPtr());
            dataCast->Append(*copyStageBuffers_->CopyStateOperation);

            EventSource::Events->CopyStreamGetNextState(
                TracePartitionId,
                ReplicaId,
                targetReplicaId_,
                copiedRecordNumber_);

            ++copiedRecordNumber_;
            result = dataCast.RawPtr();
            co_return status;
        }

        if (uptoLsn_ < beginCheckpointRecord_->LastStableLsn)
        {
            // Ensure we copy records up to the stable LSN of the begin checkpoint record as part of the copy stream
            // so that the idle can initiate a checkpoint and apply it as part of the copy pump itself

            // Not doing the above will mean that the idle can get promoted to active even before the checkpoint is completed
            // uptolsn is inclusive LSN
            // This bug is fixed with this change - BUG: RDBug #9269022:Replicator must rename copy log before changing role to active secondary during full builds
            uptoLsn_ = beginCheckpointRecord_->LastStableLsn;

            EventSource::Events->CopyStreamGetNextLogs(
                TracePartitionId,
                ReplicaId,
                targetReplicaId_,
                sourceStartingLsn_,
                beginCheckpointRecord_->LastStableLsn,
                uptoLsn_);
        }

        result = GetCopyStateMetadata();
        copyStage_ = CopyStage::Enum::CopyLog;
        copiedRecordNumber_ = 0;
        co_return status;
    }

    if (copyStage_ == CopyStage::Enum::CopyFalseProgress)
    {
        copyStage_ = CopyStage::Enum::CopyScanToStartingLsn;
        bw_.Position = 0;
        bw_.Write(sourceStartingLsn_);
        KArray<KBuffer::CSPtr> buffers(GetThisAllocator());
        KBuffer::CSPtr buf = bw_.GetBuffer(0).RawPtr();
        status = buffers.Append(buf);

        CO_RETURN_ON_FAILURE(status);

        data = OperationData::Create(buffers, GetThisAllocator());
        OperationData::SPtr dataCast = const_cast<OperationData *>(data.RawPtr());
        dataCast->Append(*copyStageBuffers_->CopyFalseProgressOperation);
        
        EventSource::Events->CopyStreamGetNextFalseProgress(
            TracePartitionId,
            ReplicaId,
            targetReplicaId_,
            sourceStartingLsn_,
            targetStartingLsn_);

        result = OperationData::CSPtr(dataCast.RawPtr());
        co_return status;
    }

    LogRecord::SPtr record;
    if (copyStage_ == CopyStage::Enum::CopyScanToStartingLsn)
    {
        LONG64 startingLsn = (sourceStartingLsn_ < targetStartingLsn_)
            ? sourceStartingLsn_
            : targetStartingLsn_;

        do
        {
            bool hasMoved = co_await logRecordsToCopy_->MoveNextAsync(cancellationToken);
            if (!hasMoved)
            {
                goto Finish;
            }

            record = logRecordsToCopy_->GetCurrent();

            ASSERT_IFNOT(
                record->Lsn <= startingLsn,
                "{0}: record lsn({1}) must be <= startingLsn({2}",
                TraceId,
                record->Lsn,
                startingLsn);

        } while (record->Lsn < startingLsn);

        // The log stream is positioned at the end of startingLsn at this point. The enumerator Current is pointing to the "startingLsn"
        copyStage_ = CopyStage::Enum::CopyLog;
    }

    if (copyStage_ == CopyStage::Enum::CopyLog)
    {
        Common::Stopwatch logPreparationWatch;
        logPreparationWatch.Start();

        OperationData::SPtr copyData = nullptr;
        LONG64 logSize = 0;
        LONG64 firstLsn = 0, lastLsn;

        do
        {
            bool moreLog = co_await logRecordsToCopy_->MoveNextAsync(cancellationToken);

            //break out of loop if no more log is left
            if (!moreLog)
            {
                break;
            }

            record = logRecordsToCopy_->GetCurrent();

            if (LogRecord::IsPhysical(*record))
            {
                continue;
            }

            if (record->Lsn > uptoLsn_)
            {
                // This bug is fixed with this change - RD: RDBug 9749316 : CopyBatch change sends more data than needed during copy
                // NOTE: Check the LSN and break right here instead of the check at the bottom because
                // the data will not be null otherwise. 
                // This will result in unnecessarily sending more copy operations than required in the stream
                break;
            }

            LogicalLogRecord::SPtr logicalLogRecord = dynamic_cast<LogicalLogRecord *>(record.RawPtr());
            OperationData::CSPtr logRecDataConst = logicalLogRecord->SerializeLogicalData();

            if (copyData == nullptr)
            {
                firstLsn = logicalLogRecord->Lsn;
                lastLsn = logicalLogRecord->Lsn;
                copyData = const_cast<OperationData *>(logRecDataConst.RawPtr());
            }
            else
            {
                lastLsn = logicalLogRecord->Lsn;
                copyData->Append(*const_cast<OperationData *>(logRecDataConst.RawPtr()));
            }
            
            logSize += OperationData::GetOperationSize(*logRecDataConst);

            if (logSize > GetCopyBatchSizeInBytes()) //Recompute every time as it is a dynamic config that we must load always
            {
                // Break if we reached uptolsn or buffered enough data
                break;
            }

        } while (true);

        //if we are out of the loop with no records to send, that means no more log is left.
        if (copyData == nullptr)
        {
            goto Finish;
        }

        ++copiedRecordNumber_;
        
        // data will not be nullptr as records.Count() is > 0
        copyData->Append(*copyStageBuffers_->CopyLogOperation);

        logPreparationWatch.Stop();

        EventSource::Events->CopyStreamGetNextLogRecord(
            TracePartitionId,
            ReplicaId,
            targetReplicaId_,
            firstLsn,
            lastLsn,
            logPreparationWatch.Elapsed,
            apiCallLatency);

        result = copyData.RawPtr();
        co_return status;
    }

Finish:
    if (logRecordsToCopy_ != nullptr)
    {
        logRecordsToCopy_->Dispose();
    }
    
    EventSource::Events->CopyStreamFinished(
        TracePartitionId,
        ReplicaId,
        targetReplicaId_,
        copiedRecordNumber_);

    co_return status;
}

OperationData::CSPtr CopyStream::GetCopyMetadata(__in CopyStage::Enum copyStageToWrite)
{
    BinaryWriter bw(GetThisAllocator());

    bw.Write(copyMetadataVersion_);
    bw.Write(static_cast<int>(copyStageToWrite));
    bw.Write(replicaId_);

    KArray<KBuffer::CSPtr> buffers(GetThisAllocator());
    THROW_ON_CONSTRUCTOR_FAILURE(buffers);
    KBuffer::SPtr buffer = bw.GetBuffer(0);
    NTSTATUS status = buffers.Append(buffer.RawPtr());
    THROW_ON_FAILURE(status);
    
    OperationData::CSPtr result = OperationData::Create(buffers, GetThisAllocator());
    
    EventSource::Events->CopyStreamMetadata(
        TracePartitionId,
        ReplicaId,
        targetReplicaId_,
        copyStage_,
        sourceStartingLsn_,
        targetStartingLsn_);

    return result;
}

OperationData::CSPtr CopyStream::GetCopyStateMetadata()
{
    ULONG32 expectedSize = beginCheckpointRecord_->CurrentProgressVector->get_ByteCount();
    expectedSize += copyStateMetadataByteSizeNotIncludingProgressVector_;

    BinaryWriter bw(GetThisAllocator());

    bw.Write(copyStateMetadataVersion_);
    beginCheckpointRecord_->CurrentProgressVector->Write(bw);
    bw.Write(beginCheckpointRecord_->EpochValue.DataLossVersion);
    bw.Write(beginCheckpointRecord_->EpochValue.ConfigurationVersion);

    bw.Write(sourceStartingLsn_);
    bw.Write(beginCheckpointRecord_->Lsn);
    bw.Write(uptoLsn_);
    bw.Write(replicatedLogManager_->CurrentLogTailLsn);

    ASSERT_IF(
        expectedSize != bw.Position,
        "{0}: Position mismatch in GetCopyStateMetadata. Expected {1} Position {2}",
        TraceId,
        expectedSize,
        bw.Position);

    KArray<KBuffer::CSPtr> buffers(GetThisAllocator());
    THROW_ON_CONSTRUCTOR_FAILURE(buffers);

    KBuffer::SPtr buffer = bw.GetBuffer(0);
    NTSTATUS status = buffers.Append(buffer.RawPtr());
    THROW_ON_FAILURE(status);

    status = buffers.Append(copyStageBuffers_->CopyProgressVectorOperation.RawPtr());

    THROW_ON_FAILURE(status);

    OperationData::CSPtr result = OperationData::Create(
        buffers,
        GetThisAllocator());

    EventSource::Events->CopyStreamStateMetadata(
        TracePartitionId,
        ReplicaId,
        targetReplicaId_,
        copyStage_,
        sourceStartingLsn_,
        beginCheckpointRecord_->Lsn,
        copiedRecordNumber_,
        replicatedLogManager_->CurrentLogTailLsn);

    return result;
}

int64 Data::LoggingReplicator::CopyStream::GetCopyBatchSizeInBytes()
{
    int64 copyBatchSizeInBytes = (transactionalReplicatorConfig_->CopyBatchSizeInKb * 1024);
    if (copyBatchSizeInBytes > maxReplicationMessageSizeInBytes_)
    {
        return 0;
    }
    else 
    {
        return copyBatchSizeInBytes;
    }
}
