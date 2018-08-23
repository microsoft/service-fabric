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
using namespace TestHooks;

Common::StringLiteral const TraceComponent("SecondaryDrainManager");

SecondaryDrainManager::SharedAtomicLong::SPtr SecondaryDrainManager::SharedAtomicLong::Create(
    __in long startingCount,
    __in KAllocator & allocator)
{
    SecondaryDrainManager::SharedAtomicLong * pointer = _new(SECONDARYDRAINMANAGER_TAG, allocator)SecondaryDrainManager::SharedAtomicLong(startingCount);
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return SecondaryDrainManager::SharedAtomicLong::SPtr(pointer);
}

SecondaryDrainManager::SharedAtomicLong::SharedAtomicLong(__in long startingCount)
    : long_(startingCount)
{
}

SecondaryDrainManager::SharedAtomicLong::~SharedAtomicLong()
{
}

long SecondaryDrainManager::SharedAtomicLong::DecrementAndGet()
{
    return --long_;
}

SecondaryDrainManager::SecondaryDrainManager(
    __in PartitionedReplicaId const & traceId,
    __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
    __in RoleContextDrainState & roleContextDrainState,
    __in OperationProcessor & recordsProcessor,
    __in IStateReplicator & fabricReplicator,
    __in IBackupManager & backupManager,
    __in CheckpointManager & checkpointManager,
    __in TransactionManager & transactionManager,
    __in ReplicatedLogManager & replicatedLogManager,
    __in IStateProviderManager & stateManager,
    __in RecoveryManager & recoveryManager,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in InvalidLogRecords & invalidLogRecords)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , replicatedLogManager_(&replicatedLogManager)
    , checkpointManager_(&checkpointManager)
    , transactionManager_(&transactionManager)
    , fabricReplicator_(&fabricReplicator)
    , backupManager_(&backupManager)
    , stateManager_(&stateManager)
    , recoveryManager_(&recoveryManager)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , roleContextDrainState_(&roleContextDrainState)
    , recordsProcessor_(&recordsProcessor)
    , copiedUptoLsn_(Constants::InvalidLsn)
    , invalidLogRecords_(&invalidLogRecords)
    , recoveredOrCopiedCheckpointState_(&recoveredOrCopiedCheckpointState)
    , readConsistentAfterLsn_(Constants::InvalidLsn)
    , testHookContext_()
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"SecondaryDrainManager",
        reinterpret_cast<uintptr_t>(this));

    Reuse();

    if (transactionalReplicatorConfig_->EnableSecondaryCommitApplyAcknowledgement == true)
    {
        // MCoskun: This is only exposed for Native Image Store so that it can replicate outside of the replicator.
        // It has multiple side effects like slowing the secondary drain and causing copy|replication queues to grow faster.
        // Furthermore, there are multiple application layer logic required to support blocking notifications even when this is enabled.
        // This is not meant to be used by customers or most internal services.
        EventSource::Events->EnableSecondaryCommitApplyAcknowledgement(
            TracePartitionId,
            ReplicaId);
    }
}

SecondaryDrainManager::~SecondaryDrainManager()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"SecondaryDrainManager",
        reinterpret_cast<uintptr_t>(this));
}

SecondaryDrainManager::SPtr SecondaryDrainManager::Create(
    __in PartitionedReplicaId const & traceId,
    __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
    __in RoleContextDrainState & roleContextDrainState,
    __in OperationProcessor & recordsProcessor,
    __in IStateReplicator & fabricReplicator,
    __in IBackupManager & backupManager,
    __in CheckpointManager & checkpointManager,
    __in TransactionManager & transactionManager,
    __in ReplicatedLogManager & replicatedLogManager,
    __in IStateProviderManager & stateManager,
    __in RecoveryManager & recoveryManager,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator)
{
    SecondaryDrainManager * pointer = _new(SECONDARYDRAINMANAGER_TAG, allocator)SecondaryDrainManager(
        traceId,
        recoveredOrCopiedCheckpointState,
        roleContextDrainState,
        recordsProcessor,
        fabricReplicator,
        backupManager,
        checkpointManager,
        transactionManager,
        replicatedLogManager,
        stateManager,
        recoveryManager,
        transactionalReplicatorConfig,
        invalidLogRecords);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return SecondaryDrainManager::SPtr(pointer);
}

void SecondaryDrainManager::Reuse()
{
    InitializeCopyAndReplicationDrainTcs();
    copiedUptoLsn_ = Constants::MaxLsn;
    readConsistentAfterLsn_ = Constants::MaxLsn;
    replicationStreamDrainCompletionTcs_->TrySet();
    copyStreamDrainCompletionTcs_->TrySet();
}

void SecondaryDrainManager::InitializeCopyAndReplicationDrainTcs()
{
    replicationStreamDrainCompletionTcs_ = CompletionTask::CreateAwaitableCompletionSource<void>(SECONDARYDRAINMANAGER_TAG, GetThisAllocator());
    copyStreamDrainCompletionTcs_ = CompletionTask::CreateAwaitableCompletionSource<void>(SECONDARYDRAINMANAGER_TAG, GetThisAllocator());
}

void SecondaryDrainManager::OnSuccessfulRecovery(
    __in LONG64 recoveredLsn)
{
    readConsistentAfterLsn_ = recoveredLsn;
}

Awaitable<void> SecondaryDrainManager::WaitForCopyAndReplicationDrainToCompleteAsync()
{
    co_await copyStreamDrainCompletionTcs_->GetAwaitable();
    co_await replicationStreamDrainCompletionTcs_->GetAwaitable();

    co_return;
}

Awaitable<void> SecondaryDrainManager::WaitForCopyDrainToCompleteAsync()
{
    co_await copyStreamDrainCompletionTcs_->GetAwaitable();
    co_return;
}

void SecondaryDrainManager::StartBuildingSecondary()
{
    // The task can start really late in stress conditions. Before completing change role, re-initialize the drain tcs to avoid racing close to go ahead of the drain
    InitializeCopyAndReplicationDrainTcs();
    BuildSecondaryAsync();
}

void SecondaryDrainManager::Test_SetTestHookContext(TestHooks::TestHookContext const & testHookContext)
{
    testHookContext_ = testHookContext;
}

Awaitable<NTSTATUS> SecondaryDrainManager::BuildSecondaryInternalAsync()
{
    IOperationStream::SPtr copyStream = nullptr;
    NTSTATUS status = fabricReplicator_->GetCopyStream(copyStream);

    if (!NT_SUCCESS(status))
    {
        LR_TRACE_EXCEPTION(
            L"SecondaryDrainManager::BuildSecondaryAsync_GetCopyStream",
            status);

        co_return status;
    }

    status = co_await CopyOrBuildReplicaAsync(*copyStream);

    if (!NT_SUCCESS(status))
    {
        LR_TRACE_EXCEPTION(
            L"SecondaryDrainManager::BuildSecondaryAsync_CopyOrBuildReplicaAsync",
            status);
        
        co_return status;
    }

    copyStreamDrainCompletionTcs_->TrySet();

    IOperationStream::SPtr replicationStream = nullptr;
    status = fabricReplicator_->GetReplicationStream(replicationStream);

    if (!NT_SUCCESS(status))
    {
        LR_TRACE_EXCEPTION(
            L"SecondaryDrainManager::BuildSecondaryAsync_GetReplicationStream",
            status);
        
        co_return status;
    }

    status = co_await DrainReplicationStreamAsync(*replicationStream);

    if (!NT_SUCCESS(status))
    {
        LR_TRACE_EXCEPTION(
            L"SecondaryDrainManager::BuildSecondaryAsync_DrainReplicationStreamAsync",
            status);
        
        co_return status;
    }

    replicationStreamDrainCompletionTcs_->TrySet();

    co_return status;
}

Task SecondaryDrainManager::BuildSecondaryAsync()
{
    KCoShared$ApiEntry();

    NTSTATUS status = co_await BuildSecondaryInternalAsync();

    if (!NT_SUCCESS(status))
    {
        copyStreamDrainCompletionTcs_->TrySet();
        replicationStreamDrainCompletionTcs_->TrySet();
        roleContextDrainState_->ReportFault();
    }

    co_return;
}

bool SecondaryDrainManager::CopiedUpdateEpoch(
    __in UpdateEpochLogRecord & record)
{
    UpdateEpochLogRecord::SPtr recordPtr = &record;
    ASSERT_IFNOT(
        roleContextDrainState_->DrainStream == DrainingStream::Enum::Copy,
        "{0}: recordsProcessor.DrainingStream = DrainingStream::Copy",
        TraceId);

    ASSERT_IFNOT(
        recordPtr->Lsn == replicatedLogManager_->CurrentLogTailLsn,
        "{0}: record.LastLogicalSequenceNumber == currentLogTailLsn",
        TraceId
    );

    EventSource::Events->CopiedUpdateEpoch(
        TracePartitionId,
        ReplicaId,
        recordPtr->EpochValue.DataLossVersion,
        recordPtr->EpochValue.ConfigurationVersion,
        recordPtr->Lsn,
        roleContextDrainState_->ReplicaRole);

    ApiSignalHelper::WaitForSignalIfSet(testHookContext_, StateProviderBeginUpdateEpochBlock);

    ProgressVectorEntry lastVector = replicatedLogManager_->ProgressVectorValue->LastProgressVectorEntry;
    if (recordPtr->EpochValue <= lastVector.CurrentEpoch)
    {
        if (replicatedLogManager_->CurrentLogTailEpoch < lastVector.CurrentEpoch)
        {
            // This case happens during full copy before the first checkpoint
            ASSERT_IFNOT(
                recordPtr->Lsn <= recoveredOrCopiedCheckpointState_->Lsn,
                "{0}: record.LastLogicalSequenceNumber <= recoveredOrCopiedCheckpointLsn",
                TraceId);

            replicatedLogManager_->AppendWithoutReplication(*recordPtr, nullptr);
            if (replicatedLogManager_->CurrentLogTailEpoch < recordPtr->EpochValue)
            {
                replicatedLogManager_->SetTailEpoch(recordPtr->EpochValue);
            }

            return false;
        }

        bool isInserted = replicatedLogManager_->ProgressVectorValue->Insert(ProgressVectorEntry(*recordPtr));
        if (isInserted)
        {
            // This case happens when a series of primaries fail before
            // a stable one completes reconfiguration
            replicatedLogManager_->AppendWithoutReplication(*recordPtr, nullptr);
        }
        else
        {
            ProcessDuplicateRecord(*recordPtr);
        }

        return false;
    }

    replicatedLogManager_->UpdateEpochRecord(*recordPtr);
    return true;
}

Awaitable<NTSTATUS> SecondaryDrainManager::CopyOrBuildReplicaAsync(
    __in IOperationStream & copyStream)
{
    IOperationStream::SPtr copyStreamPtr = &copyStream;
    IOperation::SPtr operation = nullptr;
    NTSTATUS status = co_await copyStreamPtr->GetOperationAsync(operation);

    CO_RETURN_ON_FAILURE(status);

    if (operation == nullptr)
    {
        co_return STATUS_SUCCESS;
    }

    ASSERT_IFNOT(
        operation->Data->BufferCount == 1,
        "{0}: operation.Data.Count == 1",
        TraceId);

    CopyHeader::SPtr copyHeader = CopyHeader::Deserialize(*(operation->Data), GetThisAllocator());

    operation->Acknowledge();

    if (copyHeader->CopyStageValue == CopyStage::Enum::CopyNone)
    {
        // The order of the following statements is significant
        ASSERT_IFNOT(
            roleContextDrainState_->DrainStream == DrainingStream::Enum::Invalid,
            "{0}: CopyOrBuildReplica recordProcessor.DrainingStream == DrainingStream.Invalid",
            TraceId
        );

        // Since there is no false progress stage, dispose the recovery stream
        recoveryManager_->DisposeRecoveryReadStreamIfNeeded();

        status = co_await copyStreamPtr->GetOperationAsync(operation);
        
        CO_RETURN_ON_FAILURE(status);

        ASSERT_IFNOT(
            operation == nullptr,
            "{0}: CopyOrBuildReplica operation == null",
            TraceId);

        EventSource::Events->CopyOrBuildReplicaStatus(
            TracePartitionId,
            ReplicaId,
            L"Idle replica is already current with primary replica",
            copyHeader->PrimaryReplicaId);

        co_return STATUS_SUCCESS;
    }

    BeginCheckpointLogRecord::SPtr copiedCheckpointRecord;
    bool renamedCopyLogSuccessfully = false;

    if (copyHeader->CopyStageValue == CopyStage::Enum::CopyState)
    {
        // Since there is no false progress stage, dispose the recovery stream
        recoveryManager_->DisposeRecoveryReadStreamIfNeeded();

        EventSource::Events->CopyOrBuildReplicaStatus(
            TracePartitionId,
            ReplicaId,
            L"Idle replica is copying from Primary Replica",
            copyHeader->PrimaryReplicaId);

        status = co_await DrainStateStreamAsync(*copyStreamPtr, operation);
        
        CO_RETURN_ON_FAILURE(status);

        if (operation == nullptr)
        {
            EventSource::Events->CopyOrBuildReplicaAborted(
                TracePartitionId,
                ReplicaId);

            co_return STATUS_SUCCESS;
        }

        CopyMetadata::SPtr copyMetadata = CopyMetadata::Deserialize(*(operation->Data), GetThisAllocator());

        readConsistentAfterLsn_ = copyMetadata->HighestStateProviderCopiedLsn;

        EventSource::Events->CopyOrBuildReplicaStarted(
            TracePartitionId,
            ReplicaId,
            copyMetadata->StartingLogicalSequenceNumber,
            copyMetadata->StartingEpoch.DataLossVersion,
            copyMetadata->StartingEpoch.ConfigurationVersion,
            copyMetadata->CheckpointLsn,
            copyMetadata->UptoLsn,
            copyMetadata->HighestStateProviderCopiedLsn,
            copyMetadata->ProgressVectorValue->ToString());

        transactionManager_->TransactionMapObject->Reuse();
        checkpointManager_->ResetStableLsn(copyMetadata->CheckpointLsn);
        copiedUptoLsn_ = copyMetadata->UptoLsn;

        IndexingLogRecord::SPtr newLogHead = nullptr;
        status = co_await replicatedLogManager_->InnerLogManager->CreateCopyLogAsync(copyMetadata->StartingEpoch, copyMetadata->StartingLogicalSequenceNumber, newLogHead);

        CO_RETURN_ON_FAILURE(status);

        recoveredOrCopiedCheckpointState_->Update(copyMetadata->CheckpointLsn);

        replicatedLogManager_->Reuse(
            *copyMetadata->ProgressVectorValue,
            nullptr,
            nullptr,
            invalidLogRecords_->Inv_InformationLogRecord.RawPtr(),
            *newLogHead,
            copyMetadata->StartingEpoch,
            copyMetadata->StartingLogicalSequenceNumber);

        // RD: RDBug 7475439: Utility.Assert(sourceEntry == targetEntry, "currentSourceVector == currentTargetVector");
        // UpdateEpoch lsn is same as starting lsn, so insert UE log record
        if (copyMetadata->StartingLogicalSequenceNumber == copyMetadata->ProgressVectorValue->LastProgressVectorEntry.Lsn)
        {
            UpdateEpochLogRecord::SPtr record = UpdateEpochLogRecord::Create(
                copyMetadata->ProgressVectorValue->LastProgressVectorEntry.CurrentEpoch,
                copyMetadata->ProgressVectorValue->LastProgressVectorEntry.PrimaryReplicaId,
                *invalidLogRecords_->Inv_PhysicalLogRecord,
                GetThisAllocator());
            record->Lsn = copyMetadata->StartingLogicalSequenceNumber;

            EventSource::Events->UpdateEpochRecordDueToFullCopy(
                TracePartitionId,
                ReplicaId,
                record->EpochValue.DataLossVersion,
                record->EpochValue.ConfigurationVersion,
                record->Lsn,
                roleContextDrainState_->ReplicaRole);

            // NOTE: Do not use the UpdateEpoch method on logmanager as it adds the entry to the list of progress vectors.
            // We do not want to do that here as the entry already exists
            replicatedLogManager_->AppendWithoutReplication(*record, nullptr);
        }

        copiedCheckpointRecord = nullptr;
        if (recoveredOrCopiedCheckpointState_->Lsn == copyMetadata->StartingLogicalSequenceNumber)
        {
            checkpointManager_->FirstBeginCheckpointOnIdleSecondary();
            copiedCheckpointRecord = replicatedLogManager_->LastInProgressCheckpointRecord;

            ASSERT_IFNOT(
                copiedCheckpointRecord->Lsn == recoveredOrCopiedCheckpointState_->Lsn,
                "{0}: CopyOrBuildReplica copiedCheckpointRecordLsn {1} != recoveredOrCopiedCheckpointLsn {2}",
                TraceId,
                copiedCheckpointRecord->Lsn,
                recoveredOrCopiedCheckpointState_->Lsn);

            // If this is the UptoLsn, ensure rename is done before continuing
            if (recoveredOrCopiedCheckpointState_->Lsn == copiedUptoLsn_)
            {
                // This ensures we dont get stuck waiting for stable LSN
                ASSERT_IFNOT(
                    checkpointManager_->LastStableLsn == recoveredOrCopiedCheckpointState_->Lsn,
                    "{0}: CheckpointManager.LastStableLsn {1} == this.recoveredOrCopiedCheckpointLsn.Value {2}",
                    TraceId,
                    checkpointManager_->LastStableLsn,
                    recoveredOrCopiedCheckpointState_->Lsn);

                co_await checkpointManager_->CompleteFirstCheckpointOnIdleAndRenameLog(*copiedCheckpointRecord, copiedUptoLsn_);
                renamedCopyLogSuccessfully = true;
            }
        }

        operation->Acknowledge();

        EventSource::Events->CopyAckedProgress(
            TracePartitionId,
            ReplicaId);

        status = co_await copyStreamPtr->GetOperationAsync(operation);

        CO_RETURN_ON_FAILURE(status);
    }
    else
    {
        EventSource::Events->CopyOrBuildReplicaStatus(
            TracePartitionId,
            ReplicaId,
            L"Idle replica is building from Primary Replica",
            copyHeader->PrimaryReplicaId);

        status = co_await TruncateTailIfNecessary(*copyStreamPtr, operation);

        CO_RETURN_ON_FAILURE(status);

        copiedCheckpointRecord = invalidLogRecords_->Inv_BeginCheckpointLogRecord;

        // Since the false progress stage is complete, dispose the recovery stream
        recoveryManager_->DisposeRecoveryReadStreamIfNeeded();
    }

    status = co_await DrainCopyStreamAsync(*copyStreamPtr, operation.RawPtr(), copiedCheckpointRecord.RawPtr(), renamedCopyLogSuccessfully);

    co_return status;
}

Awaitable<NTSTATUS> SecondaryDrainManager::DrainCopyStreamAsync(
    __in IOperationStream & copyStream,
    __in IOperation * operation,
    __in BeginCheckpointLogRecord * copiedCheckpointRecord,
    __in bool renamedCopyLogSuccessfully)
{
    BeginCheckpointLogRecord::SPtr copiedCheckpointRecordSPtr = copiedCheckpointRecord;
    IOperation::SPtr operationPtr = operation;
    IOperationStream::SPtr copyStreamPtr = &copyStream;

    EventSource::Events->DrainStart(
        TracePartitionId,
        ReplicaId,
        Common::wformatString(
            L"Copy stream. RenamedCopyLogSuccessfully : {0}",
            renamedCopyLogSuccessfully));

    LogicalLogRecord::SPtr lastCopiedRecord = invalidLogRecords_->Inv_LogicalLogRecord;
    LONG64 copiedOperationNumber = 0;

    // NOTE: This need NOT be a SharedAtomicLong because the local variable remains valid throughout the execution of this method
    // As a result, the same variable is not reused
    Common::atomic_long acksOutstanding(1);

    AwaitableCompletionSource<void>::SPtr allOperationsAckedTcs = CompletionTask::CreateAwaitableCompletionSource<void>(SECONDARYDRAINMANAGER_TAG, GetThisAllocator());
    NTSTATUS status = STATUS_SUCCESS;

    if (operationPtr != nullptr)
    {
        roleContextDrainState_->OnDrainCopy();

        do
        {
            OperationData::CSPtr data = operationPtr->Data;

#ifdef DBG
            ReplicatedLogManager::ValidateOperationData(*data);
#endif
            KArray<LogRecord::SPtr> copiedRecords = LogRecord::ReadFromOperationDataBatch(*data, *invalidLogRecords_, GetThisAllocator());

            ASSERT_IFNOT(
                copiedRecords.Count() > 0,
                "{0}: copy operation received with no records inside.",
                TraceId);

            AwaitableCompletionSource<void>::SPtr allRecordsFlushedTcs = CompletionTask::CreateAwaitableCompletionSource<void>(SECONDARYDRAINMANAGER_TAG, GetThisAllocator());

            // NOTE: This MUST be a SharedAtomicLong because the local variable is reused for every iteration of the while loop
            // Since there are async tasks that depend on the value of the local variable to signal the completion of the flush tcs, we must store it on the heap using a SPtr
            SharedAtomicLong::SPtr flushOutstandingCount = SharedAtomicLong::Create(copiedRecords.Count(), GetThisAllocator());

            for (ULONG32 i = 0; i < copiedRecords.Count(); i++)
            {
                lastCopiedRecord = dynamic_cast<LogicalLogRecord *>(copiedRecords[i].RawPtr());
                ASSERT_IF(lastCopiedRecord == nullptr, "Unexpected dynamic cast failure");

                co_await LogLogicalRecordOnSecondaryAsync(*lastCopiedRecord);

                BeginCheckpointLogRecord::SPtr initiatedCheckpointRecord = nullptr;
                if (copiedCheckpointRecordSPtr == nullptr)
                {
                    copiedCheckpointRecordSPtr = replicatedLogManager_->LastInProgressCheckpointRecord;
                    if (copiedCheckpointRecordSPtr != nullptr)
                    {
                        ASSERT_IFNOT(
                            copiedCheckpointRecordSPtr->Lsn == recoveredOrCopiedCheckpointState_->Lsn,
                            "{0}: CopyOrBuildReplica copiedCheckpointRecordLsn {1} == recoveredOrCopiedCheckpointLsn {2}",
                            TraceId,
                            copiedCheckpointRecordSPtr->Lsn,
                            recoveredOrCopiedCheckpointState_->Lsn);

                        initiatedCheckpointRecord = copiedCheckpointRecordSPtr;
                    }
                }

                // If pumped the last operation in the copy stream (indicated by copiedUptoLsn), rename the copy log if this was a full copy
                // as we are guranteed that the replica has all the data needed to be promoted to an active secondary and we could not have lost any state
                if (copiedCheckpointRecordSPtr != nullptr &&
                    copiedCheckpointRecordSPtr != invalidLogRecords_->Inv_BeginCheckpointLogRecord &&
                    lastCopiedRecord->Lsn == copiedUptoLsn_ &&
                    renamedCopyLogSuccessfully == false) // Copied UE record could have same LSN, so this condition is needed
                {
                    co_await checkpointManager_->CompleteFirstCheckpointOnIdleAndRenameLog(*copiedCheckpointRecordSPtr, copiedUptoLsn_);
                    renamedCopyLogSuccessfully = true;
                }

                AwaitFlushingCopiedRecordTask(
                    *lastCopiedRecord,
                    *flushOutstandingCount,
                    *allRecordsFlushedTcs);
            }

            // After successfully appending all the records into the buffer, increment the outstanding ack count
            LONG64 acksRemaining = ++acksOutstanding;

            EventSource::Events->DrainCopyReceive(
                TracePartitionId,
                ReplicaId,
                copiedOperationNumber,
                copiedRecords[0]->Lsn,
                lastCopiedRecord->Lsn,
                acksRemaining);

            ++copiedOperationNumber;

            //acknowledge copy operation after all records are flushed
            AcknowledgeCopyOperationTask(
                *allRecordsFlushedTcs,
                *operationPtr,
                acksOutstanding,
                *allOperationsAckedTcs);

            Awaitable<NTSTATUS> drainTask = copyStreamPtr->GetOperationAsync(operationPtr);

            if (!drainTask.IsComplete())
            {
                // Currently, we cannot wait for copy to finish because copy might get
                // abandoned if the primary fails and the product waits for pending
                // copy operations to get acknowledged before electing a new primary
                Awaitable<void> flushAsync = replicatedLogManager_->InnerLogManager->FlushAsync(L"DrainCopyStream.IsEmpty");
                ToTask(flushAsync);
            }

            status = co_await drainTask;

            if (!NT_SUCCESS(status))
            {
                // Do not return immediately here as we need to cancel checkpoint if needed
                break;
            }
        } while (operationPtr != nullptr);
    }

    // This is to ensure that before we continue, we cancel the first full copy checkpoint during full build
    // Without having this, it is possible that the above code throws out of this method and any lifecycle API like close might get stuck because
    // there is a pending checkpoint that is not yet fully processed

    // If the pump prematurely finishes for any reason, it means the copy log cannot be renamed
    if (copiedCheckpointRecordSPtr != nullptr &&
        copiedCheckpointRecordSPtr != invalidLogRecords_->Inv_BeginCheckpointLogRecord &&
        renamedCopyLogSuccessfully == false)
    {
        co_await checkpointManager_->CancelFirstCheckpointOnIdleDueToIncompleteCopy(*copiedCheckpointRecordSPtr, copiedUptoLsn_);
    }

    CO_RETURN_ON_FAILURE(status);

    co_await replicatedLogManager_->FlushInformationRecordAsync(
        InformationEvent::Enum::CopyFinished,
        false,
        L"DrainCopyStream.CopyFinished");

    // Awaiting processing of this record,
    // ensures that all operations in the copystream must have been applied Before we complete the drainComplationTcs.
    co_await replicatedLogManager_->LastInformationRecord->AwaitProcessing();

    co_await recordsProcessor_->WaitForLogicalRecordsProcessingAsync();

    LONG64 acksOpen = --acksOutstanding;

    ASSERT_IFNOT(
        acksOpen >= 0,
        "{0}: BuildOrCopyReplica acksOpen {1} >= 0",
        TraceId,
        acksOpen);

    if (acksOpen != 0)
    {
        // wait for all the callbacks above to finish running and acknowleding
        co_await allOperationsAckedTcs->GetAwaitable();
    }

    ASSERT_IFNOT(
        acksOutstanding.load() == 0,
        "{0}: BuildOrCopyReplica acksOutstanding == 0",
        TraceId);

    EventSource::Events->DrainCompleted(
        TracePartitionId,
        ReplicaId,
        L"Copy stream",
        L"operations",
        copiedOperationNumber,
        L"Completed",
        lastCopiedRecord->RecordType,
        lastCopiedRecord->Lsn,
        lastCopiedRecord->Psn,
        lastCopiedRecord->RecordPosition);

    co_return STATUS_SUCCESS;
}

Task SecondaryDrainManager::AwaitFlushingCopiedRecordTask(
    __in LogicalLogRecord & copiedRecord,
    __inout SharedAtomicLong & flushesOutstanding,
    __in ktl::AwaitableCompletionSource<void> & allRecordsFlushedTcs)
{
    KCoShared$ApiEntry();

    LogicalLogRecord::SPtr copiedRecordPtr = &copiedRecord;
    AwaitableCompletionSource<void>::SPtr allRecordsFlushedTcsPtr = &allRecordsFlushedTcs;
    SharedAtomicLong::SPtr flushOutstandingCountSPtr = &flushesOutstanding;

    NTSTATUS status = co_await copiedRecordPtr->AwaitFlush();

    if (NT_SUCCESS(status) && 
        transactionalReplicatorConfig_->EnableSecondaryCommitApplyAcknowledgement == true && 
        LogRecord::IsCommitLogRecord(*copiedRecordPtr))
    {
        status = co_await copiedRecordPtr->AwaitApply();
    }

    LONG64 flushesPending = flushOutstandingCountSPtr->DecrementAndGet();

    ASSERT_IFNOT(
        flushesPending >= 0,
        "{0}: AwaitFlushingCopiedRecordTask flushesPending {1} >= 0",
        TraceId,
        flushesPending);

    if (flushesPending == 0)
    {
        allRecordsFlushedTcsPtr->TrySet();
    }

    co_return;
}

Task SecondaryDrainManager::AcknowledgeCopyOperationTask(
    __in AwaitableCompletionSource<void> & allRecordsFlushedTcs,
    __in IOperation & operation,
    __inout Common::atomic_long & acksOutstanding,
    __in AwaitableCompletionSource<void> & allOperationsAckedTcs)
{
    KCoShared$ApiEntry();

    IOperation::SPtr operationPtr = &operation;
    AwaitableCompletionSource<void>::SPtr allRecordsFlushedTcsSPtr = &allRecordsFlushedTcs;
    AwaitableCompletionSource<void>::SPtr allOperationsAckedTcsSPtr = &allOperationsAckedTcs;

    try
    {
        co_await allRecordsFlushedTcsSPtr->GetAwaitable();

        operationPtr->Acknowledge();
        LONG64 acksPending = --acksOutstanding;

        ASSERT_IFNOT(
            acksPending >= 0,
            "{0}: AcknowledgeCopyOperationTask acksPending {1} >= 0",
            TraceId,
            acksPending);

        if (acksPending == 0)
        {
            allOperationsAckedTcsSPtr->TrySet();
        }
    }
    catch (Exception const &)
    {
        // ignore exceptions
    }

    co_return;
}

Awaitable<NTSTATUS> SecondaryDrainManager::DrainReplicationStreamAsync(__in IOperationStream & replicationStream)
{
    IOperationStream::SPtr replicationStreamPtr = &replicationStream;

    EventSource::Events->DrainStart(
        TracePartitionId,
        ReplicaId,
        L"Replication stream");

    AwaitableCompletionSource<void>::SPtr allOperationsAckedTcs = CompletionTask::CreateAwaitableCompletionSource<void>(SECONDARYDRAINMANAGER_TAG, GetThisAllocator());

    LogicalLogRecord::SPtr lastReplicatedRecord = invalidLogRecords_->Inv_LogicalLogRecord;
    LONG64 replicatedRecordNumber = 0;
    Common::atomic_long acksOutstanding(1);
    Common::atomic_long bytesOutstanding(0);

    roleContextDrainState_->OnDrainReplication();

    do
    {
        IOperation::SPtr operation = nullptr;
        Awaitable<NTSTATUS> drainTask = replicationStreamPtr->GetOperationAsync(operation);

        if (!drainTask.IsComplete())
        {
            Awaitable<void> flushAsync = replicatedLogManager_->InnerLogManager->FlushAsync(L"DrainReplicationStream.IsEmpty");
            ToTask(flushAsync);
        }

        NTSTATUS status = co_await drainTask;

        CO_RETURN_ON_FAILURE(status);

        if (operation != nullptr)
        {
            OperationData::CSPtr data = operation->Data;
#ifdef DBG
            ReplicatedLogManager::ValidateOperationData(*data);
#endif

            lastReplicatedRecord = dynamic_cast<LogicalLogRecord *>(LogRecord::ReadFromOperationData(*data, *invalidLogRecords_, GetThisAllocator()).RawPtr());
            ASSERT_IF(
                lastReplicatedRecord == nullptr,
                "{0}: Unexpected dynamic cast failure",
                TraceId);

            lastReplicatedRecord->Lsn = operation->SequenceNumber;

            co_await LogLogicalRecordOnSecondaryAsync(*lastReplicatedRecord);

            LONG64 acksRemaining = ++acksOutstanding;

            EventSource::Events->DrainReplicationReceive(
                TracePartitionId,
                ReplicaId,
                replicatedRecordNumber,
                lastReplicatedRecord->RecordType,
                lastReplicatedRecord->Lsn,
                acksRemaining);

            ++replicatedRecordNumber;

            LONG64 operationSize = OperationData::GetOperationSize(*data);

            LONG64 bytesRemaining = bytesOutstanding.fetch_add(static_cast<LONG>(operationSize));
            bytesRemaining += operationSize;

            AwaitFlushingReplicatedRecordTask(
                *lastReplicatedRecord,
                *operation,
                operationSize,
                acksOutstanding,
                bytesOutstanding,
                *allOperationsAckedTcs);
        }
        else
        {
            co_await replicatedLogManager_->FlushInformationRecordAsync(
                InformationEvent::Enum::ReplicationFinished,
                false,
                L"DrainReplicationStream.IsFinished");

            co_await replicatedLogManager_->LastInformationRecord->AwaitProcessing();

            co_await recordsProcessor_->WaitForLogicalRecordsProcessingAsync();

            LONG64 acksPending = --acksOutstanding;

            ASSERT_IFNOT(
                acksPending >= 0,
                "{0}: BuildOrCopyReplica acksPending >= 0",
                TraceId);

            if (acksPending != 0)
            {
                co_await allOperationsAckedTcs->GetAwaitable();
            }

            ASSERT_IFNOT(
                acksOutstanding.load() == 0,
                "{0}: BuildOrCopyReplica acksOutstanding == 0",
                TraceId);
            break;
        }
    } while (true);

    EventSource::Events->DrainCompleted(
        TracePartitionId,
        ReplicaId,
        L"Replication stream",
        L"records",
        replicatedRecordNumber,
        L"Completed",
        lastReplicatedRecord->RecordType,
        lastReplicatedRecord->Lsn,
        lastReplicatedRecord->Psn,
        lastReplicatedRecord->RecordPosition);

    co_return STATUS_SUCCESS;
}

Task SecondaryDrainManager::AwaitFlushingReplicatedRecordTask(
    __in LogicalLogRecord & replicatedRecord,
    __in IOperation & operation,
    __in LONG64 operationSize,
    __inout Common::atomic_long & acksOutstanding,
    __inout Common::atomic_long & bytesOutstanding,
    __in AwaitableCompletionSource<void> & allOperationsAckedTcs)
{
    KCoShared$ApiEntry();

    LogicalLogRecord::SPtr replicatedRecordPtr = &replicatedRecord;
    IOperation::SPtr operationPtr = &operation;
    AwaitableCompletionSource<void>::SPtr allOperationsAckedTcsPtr = &allOperationsAckedTcs;
    LONG64 acksPending = -1;

    NTSTATUS status = co_await replicatedRecordPtr->AwaitFlush();

    if (NT_SUCCESS(status) && 
        transactionalReplicatorConfig_->EnableSecondaryCommitApplyAcknowledgement == true && 
        LogRecord::IsCommitLogRecord(*replicatedRecordPtr))
    {
        status = co_await replicatedRecordPtr->AwaitApply();
    }

    if (!NT_SUCCESS(status))
    {
        acksPending = --acksOutstanding;

        // Signal the drain completion task if needed
        if (acksPending == 0)
        {
            allOperationsAckedTcsPtr->TrySet();
        }
            
        // return without acknowledging to the v1 replicator
        co_return;
    }

    acksPending = --acksOutstanding;

    LONG64 bytesPending = bytesOutstanding.fetch_sub(static_cast<LONG>(operationSize));
    bytesPending -= operationSize;

    ASSERT_IFNOT(
        (acksPending >= 0) && (bytesPending >= 0),
        "{0}: BuildOrCopyReplica (acksPending >= 0) && (bytesPending >= 0)",
        TraceId);

    if (acksPending == 0)
    {
        allOperationsAckedTcsPtr->TrySet();
    }

    operationPtr->Acknowledge();

    EventSource::Events->AwaitFlushingReplicatedRecordTask(
        TracePartitionId,
        ReplicaId,
        replicatedRecordPtr->Lsn,
        acksPending,
        bytesPending);

    co_await replicatedRecordPtr->AwaitApply();
    co_return;
}

Awaitable<NTSTATUS> SecondaryDrainManager::DrainStateStreamAsync(
    __in IOperationStream & copyStateStream,
    __out IOperation::SPtr & result)
{
    result = nullptr;
    IOperationStream::SPtr copyStateStreamPtr = &copyStateStream;

    EventSource::Events->DrainStart(
        TracePartitionId,
        ReplicaId,
        L"State stream");

    LONG64 stateRecordNumber = 0;

    IOperation::SPtr operation = nullptr;
    NTSTATUS status = co_await copyStateStreamPtr->GetOperationAsync(operation);

    CO_RETURN_ON_FAILURE(status);

    if (operation == nullptr)
    {
        co_return STATUS_SUCCESS;
    }

    roleContextDrainState_->OnDrainState();

    status = co_await stateManager_->BeginSettingCurrentStateAsync();

    CO_RETURN_ON_FAILURE(status);

    do
    {
        OperationData::CSPtr data = operation->Data;

        CopyStage::Enum copyStage;

        KBuffer::CSPtr dataBuffer = (*data)[data->BufferCount - 1];
        BinaryReader br(*dataBuffer, GetThisAllocator());
        ULONG32 copyStageValue;
        br.Read(copyStageValue);
        copyStage = static_cast<CopyStage::Enum>(copyStageValue);

        if (copyStage == CopyStage::Enum::CopyState)
        {
            KArray<KBuffer::CSPtr> buffers(GetThisAllocator());

            CO_RETURN_ON_FAILURE(buffers.Status());

            for (ULONG32 i = 0; i < data->BufferCount - 1; i++)
            {
                buffers.Append((*data)[i]);
            }

            OperationData::CSPtr copiedData = OperationData::Create(buffers, GetThisAllocator());

            EventSource::Events->DrainStateNoise(
                TracePartitionId,
                ReplicaId,
                L"Received",
                stateRecordNumber);

            status = co_await stateManager_->SetCurrentStateAsync(stateRecordNumber, *copiedData);
            
            CO_RETURN_ON_FAILURE(status);

            operation->Acknowledge();

            EventSource::Events->DrainStateNoise(
                TracePartitionId,
                ReplicaId,
                L"Acknowledged",
                stateRecordNumber);

            stateRecordNumber++;
        }
        else
        {
            ASSERT_IFNOT(
                copyStage == CopyStage::Enum::CopyProgressVector,
                "{0}: BuildOrCopyReplica copyStage == CopyStage::CopyProgressVector",
                TraceId);

            break;
        }

        status = co_await copyStateStreamPtr->GetOperationAsync(operation);

        CO_RETURN_ON_FAILURE(status);

    } while (operation != nullptr);

    //RDBug#10479578: If copy is aborted (stream returning null), EndSettingCurrentState API will not be called.
    if (operation == nullptr)
    {
        EventSource::Events->DrainCompleted(
            TracePartitionId,
            ReplicaId,
            L"State stream",
            L"records",
            stateRecordNumber,
            L"Incomplete",
            LogRecordType::Enum::Invalid,
            0,
            0,
            0);
        co_return STATUS_CANCELLED;
    }

    status = co_await stateManager_->EndSettingCurrentState();

    CO_RETURN_ON_FAILURE(status);

    EventSource::Events->DrainCompleted(
        TracePartitionId,
        ReplicaId,
        L"State stream",
        L"records",
        stateRecordNumber,
        L"Completed",
        LogRecordType::Enum::Invalid,
        0,
        0,
        0);

    result = operation;
    co_return status;
}

Awaitable<void> SecondaryDrainManager::LogLogicalRecordOnSecondaryAsync(__in LogicalLogRecord & record)
{
    LogicalLogRecord::SPtr recordPtr = &record;
    LONG64 recordLsn = recordPtr->Lsn;

    if (recordLsn < replicatedLogManager_->CurrentLogTailLsn ||
        (recordLsn == replicatedLogManager_->CurrentLogTailLsn && recordPtr->RecordType != LogRecordType::Enum::UpdateEpoch))
    {
        ProcessDuplicateRecord(*recordPtr);
        co_return;
    }

    StartLoggingOnSecondary(*recordPtr);

    if (replicatedLogManager_->InnerLogManager->ShouldThrottleWrites)
    {
        EventSource::Events->ThrottlingWrites(
            TracePartitionId,
            ReplicaId,
            L"Blocking secondary pump due to pending flush cache size");

        co_await replicatedLogManager_->InnerLogManager->FlushAsync(L"WaitForPendingFlushToComplete");

        EventSource::Events->ThrottlingWrites(
            TracePartitionId,
            ReplicaId,
            L"Continuing secondary pump as flush cache is purged");
    }

    if (recordPtr->RecordType == LogRecordType::Enum::Barrier && roleContextDrainState_->DrainStream != DrainingStream::Enum::Copy)
    {
        // Only invoke this method on barrier boundaries and only on active replication stream.
        // copy stream will continue pumping and the pending checkpoint during full copy will only complete when we finish pumping all copy operations
        // if we stop pumping, we will hit a deadlock since checkpoint waits for copy pump completion and only on copy pump completion do we complete the checkpoint

        BarrierLogRecord::SPtr barrierRecord = dynamic_cast<BarrierLogRecord *>(recordPtr.RawPtr());
        ASSERT_IF(
            barrierRecord == nullptr,
            "{0}: Unexpected dynamic cast failure",
            TraceId);

        co_await checkpointManager_->BlockSecondaryPumpIfNeeded();
    }
    co_return;
}

void SecondaryDrainManager::StartLoggingOnSecondary(__in LogicalLogRecord & record)
{
    LogicalLogRecord::SPtr recordPtr = &record;
    switch (recordPtr->RecordType)
    {
        case LogRecordType::Enum::BeginTransaction:
        case LogRecordType::Enum::Operation:
        case LogRecordType::Enum::EndTransaction:
        case LogRecordType::Enum::Backup:
        {
            LONG64 bufferedRecordsSizeBytes = replicatedLogManager_->AppendWithoutReplication(*recordPtr, transactionManager_.RawPtr());
            
            if (bufferedRecordsSizeBytes > transactionalReplicatorConfig_->MaxRecordSizeInKB * 1024)
            {
                Awaitable<void> flushAsync = replicatedLogManager_->InnerLogManager->FlushAsync(L"InitiateLogicalRecordProcessingOnSecondary BufferedRecordsSize: ");
                ToTask(flushAsync);
            }
            break;
        }
        case LogRecordType::Enum::Barrier:
        {
            BarrierLogRecord * barrierRecord = dynamic_cast<BarrierLogRecord *>(recordPtr.RawPtr());
            ASSERT_IF(
                barrierRecord == nullptr,
                "{0}: Unexpected dynamic cast failure",
                TraceId);

            checkpointManager_->AppendBarrierOnSecondary(*barrierRecord);
            break;
        }
        case LogRecordType::Enum::UpdateEpoch:
        {
            UpdateEpochLogRecord * updateEpochRecord = dynamic_cast<UpdateEpochLogRecord *>(recordPtr.RawPtr());
            ASSERT_IF(
                updateEpochRecord == nullptr,
                "{0}: Unexpected dynamic cast failure",
                TraceId);

            CopiedUpdateEpoch(*updateEpochRecord);
            break;
        }
        default:
        {
            Common::Assert::CodingError(
                "{0}: BuildOrCopyReplica Assert or Coding error with message Unexpected record type {1}.",
                TraceId,
                recordPtr->RecordType);
            break;
        }
    }

    checkpointManager_->InsertPhysicalRecordsIfNecessaryOnSecondary(copiedUptoLsn_, roleContextDrainState_->DrainStream, *recordPtr);
}

Awaitable<NTSTATUS> SecondaryDrainManager::TruncateTailAsync(__in LONG64 tailLsn)
{
    EventSource::Events->TruncateTail(
        TracePartitionId,
        ReplicaId,
        tailLsn);

    ASSERT_IFNOT(
        (roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_IDLE_SECONDARY) || (roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY),
        "{0}: BuildOrCopyReplica (roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_IDLE_SECONDARY) || (roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)",
        TraceId);

    ASSERT_IFNOT(
        checkpointManager_->get_LastStableLsn() <= tailLsn,
        "{0}: BuildOrCopyReplica lastStableLsn <= tailLsn. last stable lsn: {1}",
        TraceId,
        checkpointManager_->get_LastStableLsn());

    ApplyContext::Enum falseProgressApplyContext = ApplyContext::Enum::SecondaryFalseProgress;

    TruncateTailManager::SPtr truncateTailManager = TruncateTailManager::Create(
        *get_PartitionedReplicaId(),
        *replicatedLogManager_,
        *(transactionManager_->TransactionMapObject),
        *stateManager_,
        *backupManager_,
        tailLsn,
        falseProgressApplyContext,
        *recoveryManager_->RecoveryLogsReader,
        *invalidLogRecords_,
        GetThisAllocator());

    LogRecord::SPtr currentRecord = nullptr;
    NTSTATUS status = co_await truncateTailManager->TruncateTailAsync(currentRecord);

    CO_RETURN_ON_FAILURE(status);

    ASSERT_IFNOT(
        currentRecord->Lsn == tailLsn,
        "{0}: CopyOrBuildReplica TruncateTail: V1 replicator ensures that lsns are continous. currentLSN {1} == tailLsn {2}",
        TraceId,
        currentRecord->Lsn,
        tailLsn);

    checkpointManager_->ResetStableLsn(tailLsn);
    recoveryManager_->OnTailTruncation(tailLsn);

    // 6450429: Replicator maintains three values for supporting consistent reads and snapshot.
    // These values have to be updated as part of the false progress correction.
    // ReadConsistentAfterLsn is used to ensure that all state providers have applied the highest recovery or copy log operation
    // that they might have seen in their current checkpoint. Reading at a tailLsn below this cause inconsistent reads across state providers.
    // Since this was a FALSE PROGRESS copy, replica is still using the checkpoints it has recovered.
    // We have undone any operation that could have been false progressed in these checkpoints (Fuzzy checkpoint only) and ensured all state providers are applied to the same barrier.
    // Hence readConsistentAfterLsn is set to the new tail of the log.
    readConsistentAfterLsn_ = tailLsn;

    // lastAppliedBarrierRecord is used to get the current visibility sequence number.
    // Technically currentRecord (new tail record) may not be a barrier however it has the guarantee of a barrier:
    //      All records before tail, including the tail must have been applied.
    // Hence we the lastAppliedBarrierRecord to the new tail record.
    // Note: No other property but .Lsn can be used.
    recordsProcessor_->TruncateTail(*currentRecord);

    // Last value that is kept in the replicator (Version Manager) is the lastDispatchingBarrier.
    // This is used for read your writes support.
    // In this case it is not necessary to modify it since this replica either has not made any new progress (its own write) or it gets elected primary and before it can do anything else
    // dispatches a new barrier which will become the lastDispatchingBarrier
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> SecondaryDrainManager::TruncateTailIfNecessary(
    __in IOperationStream & copyStream,
    __out IOperation::SPtr & result)
{
    result = nullptr;
    IOperationStream::SPtr copyStreamPtr = &copyStream;
    IOperation::SPtr operation = nullptr;
    NTSTATUS status = co_await copyStreamPtr->GetOperationAsync(operation);

    CO_RETURN_ON_FAILURE(status);

    if (operation == nullptr)
    {
        co_return STATUS_SUCCESS;
    }

    OperationData::CSPtr data = operation->Data;

    CopyStage::Enum copyStage;

    KBuffer::CSPtr dataBuffer = (*data)[data->BufferCount - 1];
    BinaryReader br(*dataBuffer, GetThisAllocator());
    ULONG32 copyStageValue;
    br.Read(copyStageValue);
    copyStage = static_cast<CopyStage::Enum>(copyStageValue);

    ASSERT_IFNOT(
        copyStage == CopyStage::Enum::CopyFalseProgress || copyStage == CopyStage::Enum::CopyLog,
        "{0}: BuildOrCopyReplica copyStage == CopyStage::Enum::CopyFalseProgress || copyStage == CopyStage::Enum::CopyLog",
        TraceId);

    if (copyStage == CopyStage::Enum::CopyFalseProgress)
    {
        LONG64 sourceStartingLsn;

        KBuffer::CSPtr lDataBuffer = (*data)[0];
        BinaryReader lBr(*lDataBuffer, GetThisAllocator());
        lBr.Read(sourceStartingLsn);

        ASSERT_IFNOT(
            sourceStartingLsn < replicatedLogManager_->CurrentLogTailLsn,
            "{0}: BuildOrCopyReplica sourceStartingLsn < currentLogTailLsn",
            TraceId);

        operation->Acknowledge();

        status = co_await TruncateTailAsync(sourceStartingLsn);

        CO_RETURN_ON_FAILURE(status);

        status = co_await copyStreamPtr->GetOperationAsync(operation);
    }

    if (NT_SUCCESS(status))
    {
        result = operation;
    }

    co_return status;
}

void SecondaryDrainManager::ProcessDuplicateRecord(__in LogRecord & record)
{
    LogRecord::SPtr recordPtr = &record;

    recordPtr->CompletedFlush(STATUS_SUCCESS);
    recordPtr->CompletedApply(STATUS_SUCCESS);
    recordPtr->CompletedProcessing();
}

