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

LoggingReplicatorImpl::LoggingReplicatorImpl(
    __in PartitionedReplicaId const & traceId,
    __in IRuntimeFolders const & runtimeFolders,
    __in IStatefulPartition & partition,
    __in IStateReplicator & stateReplicator,
    __in InvalidLogRecords & invalidLogRecords,
    __in IStateProviderManager & stateManager,
    __in KString const & logDirectory,
    __in LogManager & logManager,
    __in RoleContextDrainState & roleContextDrainState,
    __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
    __in ReplicatedLogManager & replicatedLogManager,
    __in IInternalVersionManager & versionManager,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in SLInternalSettingsSPtr const & ktlLoggerSharedLogConfig,
    __in IDataLossHandler & dataLossHandler,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in ITransactionalReplicator & transactionalReplicator,
    __in bool hasPersistedState,
    __out IStateProvider::SPtr & stateProvider)
    : ILoggingReplicator()
    , KObject()
    , KShared()
    , KWeakRefType<LoggingReplicatorImpl>()
    , PartitionedReplicaTraceComponent(traceId)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , ktlLoggerSharedLogConfig_(ktlLoggerSharedLogConfig)
    , runtimeFolders_(&runtimeFolders)
    , partition_(&partition)
    , invalidLogRecords_(&invalidLogRecords)
    , stateManager_(&stateManager)
    , dataLossHandler_(&dataLossHandler)
    , logDirectory_(&logDirectory)
    , logManager_(&logManager)
    , roleContextDrainState_(&roleContextDrainState)
    , recoveredOrCopiedCheckpointState_(&recoveredOrCopiedCheckpointState)
    , replicatedLogManager_(&replicatedLogManager)
    , versionManager_(&versionManager)
    , copyStageBuffers_(CopyStageBuffers::Create(GetThisAllocator()))
    , logFlushCallbackManager_(PhysicalLogWriterCallbackManager::Create(traceId, GetThisAllocator()))
    , logTruncationManager_(nullptr)
    , checkpointManager_(nullptr)
    , logRecordsDispatcher_(nullptr)
    , operationProcessor_(nullptr)
    , transactionManager_(nullptr)
    , recoveryManager_()
    , secondaryDrainManager_(nullptr)
    , isDisposed_(false)
    , primaryStatus_(PrimaryElectionStatus::None)
    , flushWaitLock_()
    , primaryStatusLock_()
    , changeHandlerLock_()
    , primaryTransactionsAbortingTask_(nullptr)
    , primaryTransactionsAbortingCts_(nullptr)
    , changeHandlerCache_(nullptr)
    , waitingStreams_(GetThisAllocator())
    , perfCounters_(perfCounters)
    , logRecoveryWatch_()
    , stateManagerRecoveryWatch_()
    , healthClient_(healthClient)
    , iStateReplicator_(&stateReplicator)
    , iTransactionalReplicator_(transactionalReplicator)
    , hasPersistedState_(hasPersistedState)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        Common::wformatString("LoggingReplicatorImpl \r\n {0}",
            transactionalReplicatorConfig->ToString()),
        reinterpret_cast<uintptr_t>(this));

    THROW_ON_CONSTRUCTOR_FAILURE(waitingStreams_);

    KAllocator & allocator = GetThisAllocator();

    logTruncationManager_ = LogTruncationManager::Create(
        traceId,
        *replicatedLogManager_,
        transactionalReplicatorConfig_,
        allocator);

    backupManager_ = BackupManager::Create(
        traceId,
        logDirectory,
        *stateManager_,
        replicatedLogManager,
        *logManager_,
        transactionalReplicatorConfig,
        healthClient_,
        invalidLogRecords,
        GetThisAllocator());

    recoveryManager_ = RecoveryManager::Create(
        traceId,
        *logManager_,
        *logFlushCallbackManager_,
        *invalidLogRecords_,
        GetThisAllocator());

    stateProvider_ = StateProvider::Create(
        *this,
        allocator);

    stateProvider = stateProvider_.RawPtr();
}

LoggingReplicatorImpl::~LoggingReplicatorImpl()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"LoggingReplicatorImpl",
        reinterpret_cast<uintptr_t>(this));

    ASSERT_IFNOT(isDisposed_, "Logging replicator is not disposed in destructor");
}

LoggingReplicatorImpl::SPtr LoggingReplicatorImpl::Create(
    __in PartitionedReplicaId const & traceId,
    __in IRuntimeFolders const & runtimeFolders,
    __in IStatefulPartition & partition,
    __in IStateReplicator & stateReplicator,
    __in IStateProviderManager & stateManager,
    __in KString const & logDirectory,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in SLInternalSettingsSPtr const & ktlLoggerSharedLogConfig,
    __in Data::Log::LogManager & dataLogManager,
    __in IDataLossHandler & dataLossHandler,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in ITransactionalReplicator & transactionalReplicator,
    __in bool hasPersistedState,
    __in KAllocator & allocator,
    __out IStateProvider::SPtr & stateProvider)
{
    UNREFERENCED_PARAMETER(dataLogManager);
    InvalidLogRecords::SPtr invalidLogRecords = InvalidLogRecords::Create(allocator);

    RecoveredOrCopiedCheckpointState::SPtr recoveredOrCopiedCheckpointState = RecoveredOrCopiedCheckpointState::Create(allocator);

    RoleContextDrainState::SPtr roleContextDrainState = RoleContextDrainState::Create(partition, allocator);

    VersionManager::SPtr versionManager = VersionManager::Create(allocator).RawPtr();

    LogManager::SPtr logManager;

    if (hasPersistedState == false)
    {
        logManager = up_cast<LogManager, MemoryLogManager>(MemoryLogManager::Create(
            traceId,
            perfCounters,
            healthClient,
            transactionalReplicatorConfig,
            allocator));
    }
    else if (transactionalReplicatorConfig->Test_LoggingEngine.compare(Constants::Test_Ktl_LoggingEngine) == 0)
    {
        KLogManager::SPtr ktlLogManager = KLogManager::Create(
            traceId,
            runtimeFolders,
            dataLogManager,
            transactionalReplicatorConfig,
            ktlLoggerSharedLogConfig,
            perfCounters,
            healthClient,
            allocator).RawPtr();

        logManager = ktlLogManager.RawPtr();
    }
    else if (transactionalReplicatorConfig->Test_LoggingEngine.compare(Constants::Test_File_LoggingEngine) == 0)
    {
        logManager = up_cast<LogManager, FileLogManager>(FileLogManager::Create(
            traceId,
            perfCounters,
            healthClient,
            transactionalReplicatorConfig,
            allocator));
    }
    else
    {
        // throw error, setting validation means this should never have been reached
        throw Exception(STATUS_INVALID_PARAMETER);
    }

    ReplicatedLogManager::SPtr replicatedLogManager = ReplicatedLogManager::Create(
        traceId,
        *logManager,
        *recoveredOrCopiedCheckpointState,
        nullptr,
        *roleContextDrainState,
        stateReplicator,
        *invalidLogRecords,
        allocator);

    LoggingReplicatorImpl::SPtr result = _new(LOGGINGREPLICATOR_TAG, allocator) LoggingReplicatorImpl(
        traceId,
        runtimeFolders,
        partition,
        stateReplicator,
        *invalidLogRecords,
        stateManager,
        logDirectory,
        *logManager,
        *roleContextDrainState,
        *recoveredOrCopiedCheckpointState,
        *replicatedLogManager,
        *versionManager,
        transactionalReplicatorConfig,
        ktlLoggerSharedLogConfig,
        dataLossHandler,
        perfCounters,
        healthClient,
        transactionalReplicator,
        hasPersistedState,
        stateProvider);
    THROW_ON_ALLOCATION_FAILURE(result);

    return Ktl::Move(result);
}

bool LoggingReplicatorImpl::get_HasPersistedState() const
{
    return hasPersistedState_;
}


Awaitable<NTSTATUS> LoggingReplicatorImpl::OpenAsync(__out RecoveryInformation & recoveryInformation) noexcept
{
    NTSTATUS status = co_await OpenAsync(nullptr, recoveryInformation);
    co_return status;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::OpenAsync(
    __in_opt IBackupFolderInfo const * const backupFolderInfoPtr,
    __out RecoveryInformation & recoveryInformation) noexcept
{
    bool shouldLocalStateBeRemoved = backupManager_->ShouldCleanState();

    EventSource::Events->Open(
        TracePartitionId,
        ReplicaId,
        shouldLocalStateBeRemoved);

    // Start measuring log recovery
    logRecoveryWatch_.Start();

    CreateAllManagers();

    NTSTATUS status = co_await logManager_->InitializeAsync(*logDirectory_);
    CO_RETURN_ON_FAILURE(status);

    // Delete the log and re-initialize the logging replicator if the restore token exists.
    if (shouldLocalStateBeRemoved == true)
    {
        status = co_await logManager_->DeleteLogAsync();
        CO_RETURN_ON_FAILURE(status);

        Reuse();
        CreateAllManagers();

        status = co_await logManager_->InitializeAsync(*logDirectory_);
        CO_RETURN_ON_FAILURE(status);
    }

    RecoveryPhysicalLogReader::SPtr recoveryReader = nullptr;
    if (backupFolderInfoPtr == nullptr)
    {
        status = co_await logManager_->OpenAsync(recoveryReader);
        CO_RETURN_ON_FAILURE(status);
    }
    else
    {
        // TODO: Add a RestoreBatchSizeInKB into the configuration.
        status = co_await logManager_->OpenWithRestoreFilesAsync(
            backupFolderInfoPtr->LogPathList, 
            transactionalReplicatorConfig_->CopyBatchSizeInKb,
            *invalidLogRecords_,
            CancellationToken::None, 
            recoveryReader);
        CO_RETURN_ON_FAILURE(status);
    }

    recoveryInformation = co_await recoveryManager_->OpenAsync(
        shouldLocalStateBeRemoved,
        *recoveryReader,
        false);

    // Pause log recovery measurement
    logRecoveryWatch_.Stop();

    // Start measuring state provider recovery
    stateManagerRecoveryWatch_.Start();

    co_return status;
}
            
Awaitable<NTSTATUS> LoggingReplicatorImpl::PerformRecoveryAsync(__in RecoveryInformation const & recoveryInfo) noexcept
{
    EventSource::Events->Recover(
        TracePartitionId,
        ReplicaId);

    // Finish measuring state provider recovery
    stateManagerRecoveryWatch_.Stop();
    perfCounters_->StateManagerRecovery.Value = stateManagerRecoveryWatch_.Elapsed.Seconds;
    stateManagerRecoveryWatch_.Reset();

    roleContextDrainState_->OnRecovery();

    if (recoveryInfo.ShouldRemoveLocalStateDueToIncompleteRestore)
    {
        try
        {
            backupManager_->StateCleaningComplete();
        }
        catch (Exception e)
        {
            SharedException::CSPtr exceptionSPtr = SharedException::Create(e, GetThisAllocator());

            CODING_ASSERT("{0}: Unexpected exception: {1}", TraceId, exceptionSPtr->StackTrace);
        }
    }

    // Resume log recovery measurement
    logRecoveryWatch_.Start();

    NTSTATUS status = co_await recoveryManager_->PerformRecoveryAsync(
        *recoveredOrCopiedCheckpointState_,
        *operationProcessor_,
        *checkpointManager_,
        *transactionManager_,
        *logRecordsDispatcher_,
        *replicatedLogManager_,
        (transactionalReplicatorConfig_->ReadAheadCacheSizeInKb * 1024),
        healthClient_,
        transactionalReplicatorConfig_);

    // Finish measuring recovery time, record value
    logRecoveryWatch_.Stop();
    perfCounters_->LogRecovery.Value = logRecoveryWatch_.Elapsed.Seconds;
    logRecoveryWatch_.Reset();

    CO_RETURN_ON_FAILURE(status);

    secondaryDrainManager_->OnSuccessfulRecovery(recoveryManager_->RecoveredLsn);

    roleContextDrainState_->OnRecoveryCompleted();

    ASSERT_IFNOT(
        NT_SUCCESS(recoveryManager_->RecoveryError),
        "{0}: Recovery error must be null. It is {1:x}",
        TraceId,
        recoveryManager_->RecoveryError);

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::PrepareForRestoreAsync() noexcept
{
    NTSTATUS status = co_await logManager_->DeleteLogAsync();

    // To ensure Replicator LSN is reset to 0
    Reuse();
    co_return status;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::CloseAsync() noexcept
{
    EventSource::Events->Close(
        TracePartitionId,
        ReplicaId,
        recoveryManager_->IsRemovingStateAfterOpen);

    NTSTATUS status = STATUS_SUCCESS;

    // Prevent further timer events from firing
    checkpointManager_->CancelPeriodicCheckpointTimer();

    // Do not log any more after changing role to none as the log file could have been deleted
    if (roleContextDrainState_->ReplicaRole != FABRIC_REPLICA_ROLE_NONE &&
        !recoveryManager_->IsRemovingStateAfterOpen &&
        !backupManager_->IsLogUnavailableDueToRestore())
    {
        co_await secondaryDrainManager_->WaitForCopyAndReplicationDrainToCompleteAsync();

        co_await PrepareForReplicaRoleTransitionAsync(FABRIC_REPLICA_ROLE_UNKNOWN);

        // Abort pending checkpoint/truncation AFTER role transition to ensure that a new checkpoint is not
        // accepted in the race between aborting the current checkpoint and a new barrier being enqueued

        // This also implies PrepareForRole method should never wait for physical log records processing as that will never happen
        // until the pending checkpoint is cancelled
        // RD: RDBug 10187755: Close stuck on LR because new checkpoint got inititated after close started
        checkpointManager_->AbortPendingCheckpoint();
        checkpointManager_->AbortPendingLogHeadTruncation();

        co_await logManager_->FlushAsync(L"CloseAsync");
        co_await replicatedLogManager_->LastInformationRecord->AwaitProcessing();
        co_await operationProcessor_->WaitForPhysicalRecordsProcessingAsync();

        // Log can only be closed all incoming operations have been stopped (Close) & all of the current logs have been processed.
        status = co_await logManager_->CloseLogAsync();
    }

    co_return status;
}

bool LoggingReplicatorImpl::IsReadable() noexcept
{
    if (roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_PRIMARY)
    {
        FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
        partition_->GetReadStatus(&readStatus);
        
        return (readStatus == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
    }

    return (operationProcessor_->LastAppliedBarrierRecord->Lsn >= secondaryDrainManager_->ReadConsistentAfterLsn);
}

NTSTATUS LoggingReplicatorImpl::GetLastStableSequenceNumber(__out LONG64 & lsn) noexcept
{
    lsn = checkpointManager_->LastStableLsn;
    return STATUS_SUCCESS;
}

NTSTATUS LoggingReplicatorImpl::GetLastCommittedSequenceNumber(__out LONG64 & lsn) noexcept
{
    lsn = replicatedLogManager_->CurrentLogTailLsn;
    return STATUS_SUCCESS;
}

 NTSTATUS LoggingReplicatorImpl::GetCurrentEpoch(__out FABRIC_EPOCH & result) noexcept
{
    Epoch e = replicatedLogManager_->CurrentLogTailEpoch;

    result.DataLossNumber = e.DataLossVersion;
    result.ConfigurationNumber = e.ConfigurationVersion;
    result.Reserved = NULL;

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::BackupAsync(
    __in IBackupCallbackHandler & backupCallbackHandler,
    __out BackupInfo & result) noexcept
{
    KShared$ApiEntry();

    if (hasPersistedState_ == false)
    {
        result = {};
        // Currently, backup/restore is not supported for in-memory stack
        co_return STATUS_NOT_SUPPORTED;
    }

    NTSTATUS status = ErrorIfNoWriteStatus();

    CO_RETURN_ON_FAILURE(status);

    try
    {
        result = co_await backupManager_->BackupAsync(
            backupCallbackHandler);
    }
    catch (Exception const e)
    {
        status = e.GetStatus();
    }

    co_return status;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::BackupAsync(
    __in IBackupCallbackHandler & backupCallbackHandler,
    __in FABRIC_BACKUP_OPTION backupOption,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken,
    __out BackupInfo & result) noexcept
{
    KShared$ApiEntry();

    if (hasPersistedState_ == false)
    {
        result = {};
        // Currently, backup/restore is not supported for in-memory stack
        co_return STATUS_NOT_SUPPORTED;
    }

    NTSTATUS status = ErrorIfNoWriteStatus();

    CO_RETURN_ON_FAILURE(status);

    try
    {
        result = co_await backupManager_->BackupAsync(
            backupCallbackHandler, 
            backupOption, 
            timeout, 
            cancellationToken);
    }
    catch (Exception const e)
    {
        status = e.GetStatus();
    }

    co_return status;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::RestoreAsync(
    __in KString const & backupFolder) noexcept
{
    KShared$ApiEntry();

    if (hasPersistedState_ == false)
    {
        // Currently, restore is not supported for in-memory stack
        co_return STATUS_NOT_SUPPORTED;
    }

    NTSTATUS status = STATUS_SUCCESS;
    
    try
    {
        co_await backupManager_->RestoreAsync(backupFolder);
    }
    catch (Exception const & e)
    {
        status = e.GetStatus();
    }

    co_return status;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::RestoreAsync(
    __in KString const & backupFolder,
    __in FABRIC_RESTORE_POLICY restorePolicy,
    __in Common::TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    if (hasPersistedState_ == false)
    {
        // Currently, restore is not supported for in-memory stack
        co_return STATUS_NOT_SUPPORTED;
    }

    NTSTATUS status = STATUS_SUCCESS;

    try
    {
        co_await backupManager_->RestoreAsync(backupFolder, restorePolicy, timeout, cancellationToken);
    }
    catch (Exception const & e)
    {
        status = e.GetStatus();
    }

    co_return status;
}

NTSTATUS LoggingReplicatorImpl::GetVersion(__out LONG64 & version) const noexcept
{
    version = operationProcessor_->LastAppliedBarrierRecord->Lsn;

    ASSERT_IFNOT(
        version > FABRIC_INVALID_SEQUENCE_NUMBER + 1, 
        "{0}: Invalid version: {1}", 
        TraceId, 
        version);

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::TryRemoveCheckpointAsync(
    __in LONG64 checkpointLsnToBeRemoved, 
    __in LONG64 nextCheckpointLsn) noexcept
{
    NTSTATUS status = co_await versionManager_->TryRemoveCheckpointAsync(checkpointLsnToBeRemoved, nextCheckpointLsn);

    co_return status;
}

NTSTATUS LoggingReplicatorImpl::TryRemoveVersion(
    __in LONG64 stateProviderId,
    __in LONG64 commitLsn,
    __in LONG64 nextCommitLsn,
    __out TryRemoveVersionResult::SPtr & result) noexcept
{
    return versionManager_->TryRemoveVersion(stateProviderId, commitLsn, nextCommitLsn, result);
}

//
// This method ensures that the first ever replicated record on a new primary is a barrier
//
NTSTATUS LoggingReplicatorImpl::ReplicateBarrierIfPrimaryStatusIsBelowEstablished() noexcept
{
    BarrierLogRecord::SPtr record = nullptr;

    if (primaryStatus_ <= PrimaryElectionStatus::Elected)
    {
        // Check outside the lock since the condition is false most of the times

        K_LOCK_BLOCK(primaryStatusLock_)
        {
            if (primaryStatus_ <= PrimaryElectionStatus::Elected)
            {
                NTSTATUS status = checkpointManager_->ReplicateBarrier(record);

                RETURN_ON_FAILURE(status);

                primaryStatus_ = PrimaryElectionStatus::Established;
            }
        }

        if (record != nullptr)
        {
            // This is needed to keep the record alive
            Awaitable<void> task = AwaitProcessingBarrierRecord(*record);
            ToTask(task);
        }
    }

    return STATUS_SUCCESS;
}

//
// This method ensures that the first ever replicated record on a new primary is a barrier
//
Awaitable<NTSTATUS> LoggingReplicatorImpl::ReplicateBarrierIfPrimaryStatusIsBelowEstablishedAsync() noexcept
{
    BarrierLogRecord::SPtr record = nullptr;

    if (primaryStatus_ <= PrimaryElectionStatus::Elected)
    {
        // Check outside the lock since the condition is false most of the times

        K_LOCK_BLOCK(primaryStatusLock_)
        {
            if (primaryStatus_ <= PrimaryElectionStatus::Elected)
            {
                NTSTATUS status = checkpointManager_->ReplicateBarrier(record);

                CO_RETURN_ON_FAILURE(status);

                primaryStatus_ = PrimaryElectionStatus::Established;
            }
        }

        if (record != nullptr)
        {
            co_await record->AwaitProcessing();
        }
    }

    co_return STATUS_SUCCESS;
}

Awaitable<void> LoggingReplicatorImpl::AwaitProcessingBarrierRecord(BarrierLogRecord & barrierLogRecord) noexcept
{
    // Adding this because callers do not await this and instead do a ToTask
    KCoShared$ApiEntry();

    BarrierLogRecord::SPtr barrierLogRecordSPtr = &barrierLogRecord;

    co_await barrierLogRecordSPtr->AwaitProcessing();

    co_return;
}

NTSTATUS LoggingReplicatorImpl::BeginTransaction(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);

    NTSTATUS status = ReplicateBarrierIfPrimaryStatusIsBelowEstablished();

    RETURN_ON_FAILURE(status);

    return transactionManager_->BeginTransaction(
        transaction,
        metaData,
        undo,
        redo,
        operationContext);
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::BeginTransactionAsync(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & commitLsn) noexcept
{
    commitLsn = Constants::InvalidLsn;

    UNREFERENCED_PARAMETER(stateProviderId);

    NTSTATUS status = ReplicateBarrierIfPrimaryStatusIsBelowEstablished();

    CO_RETURN_ON_FAILURE(status);

    status = co_await transactionManager_->BeginTransactionAsync(
        transaction,
        metaData,
        undo,
        redo,
        operationContext,
        commitLsn);

    CO_RETURN_ON_FAILURE(status);

    perfCounters_->AvgCommitLatency.IncrementBy(transaction.CommitLatencyWatch.ElapsedMilliseconds);
    perfCounters_->AvgCommitLatencyBase.Increment();

    co_return status;
}

NTSTATUS LoggingReplicatorImpl::AddOperation(
    __in Transaction & transaction, 
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);

    NTSTATUS status = ReplicateBarrierIfPrimaryStatusIsBelowEstablished();

    RETURN_ON_FAILURE(status);

    return transactionManager_->AddOperation(
        transaction,
        metaData,
        undo,
        redo,
        operationContext);
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::AddOperationAsync(
    __in AtomicOperation & atomicOperation,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & commitLsn) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);

    commitLsn = Constants::InvalidLsn;

    NTSTATUS status = ReplicateBarrierIfPrimaryStatusIsBelowEstablished();

    CO_RETURN_ON_FAILURE(status);

    status = co_await transactionManager_->AddOperationAsync(
        atomicOperation,
        metaData,
        undo,
        redo,
        operationContext,
        commitLsn);

    co_return status;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::AddOperationAsync(
    __in AtomicRedoOperation & atomicRedoOperation,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const redo,
    __in LONG64 stateProviderId,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & commitLsn) noexcept
{
    UNREFERENCED_PARAMETER(stateProviderId);

    commitLsn = Constants::InvalidLsn;

    NTSTATUS status = ReplicateBarrierIfPrimaryStatusIsBelowEstablished();

    CO_RETURN_ON_FAILURE(status);

    status = co_await transactionManager_->AddOperationAsync(
        atomicRedoOperation,
        metaData,
        redo,
        operationContext,
        commitLsn);

    co_return status;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::CommitTransactionAsync(
    __in Transaction & transaction,
    __out LONG64 & commitLsn) noexcept
{
    commitLsn = Constants::InvalidLsn;

    NTSTATUS status = ReplicateBarrierIfPrimaryStatusIsBelowEstablished();

    CO_RETURN_ON_FAILURE(status);

    status = co_await transactionManager_->CommitTransactionAsync(
        transaction,
        commitLsn);

    CO_RETURN_ON_FAILURE(status);

    perfCounters_->AvgCommitLatency.IncrementBy(transaction.CommitLatencyWatch.ElapsedMilliseconds);
    perfCounters_->AvgCommitLatencyBase.Increment();

    co_return status;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::AbortTransactionAsync(
    __in Transaction & transaction,
    __out LONG64 & commitLsn) noexcept
{ 
    NTSTATUS status = ReplicateBarrierIfPrimaryStatusIsBelowEstablished();

    CO_RETURN_ON_FAILURE(status);

    status = co_await transactionManager_->AbortTransactionAsync(
        transaction,
        commitLsn);

    co_return status;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::RegisterAsync(__out LONG64 & vsn) noexcept
{
    perfCounters_->InflightVisibilitySequenceNumberCount.Increment();

    // Assumption: calling layer must ensure that the LRImpl is kept alive.
    NTSTATUS status = co_await versionManager_->RegisterAsync(vsn);

    co_return status;
}

NTSTATUS LoggingReplicatorImpl::UnRegister(LONG64 visibilityVersionNumber) noexcept
{
    perfCounters_->InflightVisibilitySequenceNumberCount.Decrement();

    return versionManager_->UnRegister(visibilityVersionNumber);
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::BecomeActiveSecondaryAsync(StateManagerBecomeSecondaryDelegate stateManagerBecomeSecondaryDelegate) noexcept
{
    EventSource::Events->BecomeSecondary(
        TracePartitionId,
        ReplicaId,
        true);

    NTSTATUS status = STATUS_SUCCESS;

    ASSERT_IFNOT(
        !recoveryManager_->IsRemovingStateAfterOpen,
        "{0}: Cannot transition to idle because remove state is pending",
        TraceId);

    ASSERT_IFNOT(
        NT_SUCCESS(recoveryManager_->RecoveryError),
        "{0}: Recovery error must be null. It is {1:x}",
        TraceId,
        recoveryManager_->RecoveryError);

    FABRIC_REPLICA_ROLE previousRole = co_await PrepareForReplicaRoleTransitionAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);

    if (previousRole != FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
    {
        secondaryDrainManager_->StartBuildingSecondary();
    }

    status = co_await logRecordsDispatcher_->DrainAndPauseDispatchAsync();

    if (!NT_SUCCESS(status))
    {
        logRecordsDispatcher_->ContinueDispatch();
        co_return status;
    }

    status = co_await stateManagerBecomeSecondaryDelegate(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

    if (!NT_SUCCESS(status))
    {
        operationProcessor_->ProcessServiceError(
            L"BecomeActiveSecondary on StateManager",
            *invalidLogRecords_->Inv_LogRecord,
            status);
    }

    logRecordsDispatcher_->ContinueDispatch();
    co_return status;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::BecomeIdleSecondaryAsync() noexcept
{
    EventSource::Events->BecomeSecondary(
        TracePartitionId,
        ReplicaId,
        false);

    ASSERT_IFNOT(
        !recoveryManager_->IsRemovingStateAfterOpen,
        "{0}: Cannot transition to idle because remove state is pending",
        TraceId);

    ASSERT_IFNOT(
        NT_SUCCESS(recoveryManager_->RecoveryError),
        "{0}: Recovery error must be null. It is {1:x}",
        TraceId,
        recoveryManager_->RecoveryError);

    FABRIC_REPLICA_ROLE previousRole = co_await PrepareForReplicaRoleTransitionAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY);

    if (previousRole != FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
    {
        secondaryDrainManager_->StartBuildingSecondary();
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::BecomeNoneAsync() noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    EventSource::Events->BecomeNone(
        TracePartitionId,
        ReplicaId,
        recoveryManager_->IsRemovingStateAfterOpen);

    co_await secondaryDrainManager_->WaitForCopyAndReplicationDrainToCompleteAsync();
    // This is needed when replica opens and CR->None without other change roles. Wait for copy stream to drain as it could race the dispose
    recoveryManager_->DisposeRecoveryReadStreamIfNeeded();

    co_await PrepareForReplicaRoleTransitionAsync(FABRIC_REPLICA_ROLE_NONE);

    // Abort pending checkpoint/truncation AFTER role transition to ensure that a new checkpoint is not
    // accepted in the race between aborting the current checkpoint and a new barrier being enqueued

    // This also implies PrepareForRole method should never wait for physical log records processing as that will never happen
    // until the pending checkpoint is cancelled
    // RD: RDBug 10187755: Close stuck on LR because new checkpoint got inititated after close started
    checkpointManager_->AbortPendingCheckpoint();
    checkpointManager_->AbortPendingLogHeadTruncation();

    if (!recoveryManager_->IsRemovingStateAfterOpen)
    {
        co_await logManager_->FlushAsync(L"BecomeNoneAsync");
        co_await replicatedLogManager_->LastInformationRecord->AwaitProcessing();

        co_await operationProcessor_->WaitForPhysicalRecordsProcessingAsync();
    }

    status = backupManager_->DeletePersistedState();
    if (NT_SUCCESS(status) == false)
    {
        LR_TRACE_UNEXPECTEDEXCEPTION_STATUS(L"BackupManager::DeletePersistedState", status);
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::BecomePrimaryAsync(bool isForRestore) noexcept
{
    EventSource::Events->BecomePrimary(
        TracePartitionId,
        ReplicaId);

    ASSERT_IFNOT(
        !recoveryManager_->IsRemovingStateAfterOpen,
        "{0}: Cannot transition to primary because remove state is pending",
        TraceId);

    ASSERT_IFNOT(
        NT_SUCCESS(recoveryManager_->RecoveryError),
        "{0}: Recovery error must be null. It is {1:x}",
        TraceId,
        recoveryManager_->RecoveryError);

    co_await secondaryDrainManager_->WaitForCopyAndReplicationDrainToCompleteAsync();
    recoveryManager_->DisposeRecoveryReadStreamIfNeeded();

    co_await QuiesceReplicaActivityAsync();

    KSharedArray<BeginTransactionOperationLogRecord::SPtr>::SPtr abortingTransactions;
    abortingTransactions = _new(LOGGINGREPLICATOR_TAG, GetThisAllocator()) KSharedArray<BeginTransactionOperationLogRecord::SPtr>();

    transactionManager_->TransactionMapObject->GetPendingTransactions(*abortingTransactions);

    for (ULONG i = 0; i < abortingTransactions->Count(); i++)
    {
        EventSource::Events->BecomePrimaryAbortTransaction(
            TracePartitionId,
            ReplicaId,
            (*abortingTransactions)[i]->RecordType,
            (*abortingTransactions)[i]->Lsn,
            (*abortingTransactions)[i]->Psn,
            (*abortingTransactions)[i]->RecordPosition,
            (*abortingTransactions)[i]->BaseTransaction.TransactionId);
    }

    // MCoskun:
    // Check if this replica is retaining its Primary Status: ChangeRole U->P, UpdateEpoch has not been called and Restore is not in flight.
    // Note that we know UpdateEpoch has not been called yet if the replica has progress higher than the
    // lsn in the last progress ProgressVectorEntry which contains the latest Epoch and highest lsn seen in the previous Epoch.
    // If Update Epoch was called, recovered lsn and highest lsn seen in the previous Epoch would match.
    // Note that during Restore, BecomePrimaryAsync is initiated by BackupManager and not by Reliability.
    if ((roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_UNKNOWN) && 
        (recoveryManager_->RecoveredLsn > replicatedLogManager_->ProgressVectorValue->LastProgressVectorEntry.Lsn)
        && !isForRestore)
    {
        // MCoskun:
        // There is a future Reliability improvement to not do a reconfiguration when an entire partition restarts.
        // Instead the previous Primary will come up and continue where it left of.
        // Since there is no reconfiguration, that means that UpdateEpoch would not be called.
        // In the absence of UpdateEpoch (which is a barrier), the replica that is retaining its primary status needs to inject its own.
        // PrimaryStatus.Retained is a primary status reserved for a replica becoming primary after failing as a primary without a reconfiguration.
        // Since this feature in reliability is not implemented yet, this code path should never execute.
        Common::Assert::CodingError(
            "{0}: TEST: Retained: Recovered Lsn: {1} LastProgressVectorEntry: <{2}, {3}> {4}",
            TraceId,
            recoveryManager_->RecoveredLsn,
            replicatedLogManager_->ProgressVectorValue->LastProgressVectorEntry.CurrentEpoch.DataLossVersion,
            replicatedLogManager_->ProgressVectorValue->LastProgressVectorEntry.CurrentEpoch.ConfigurationVersion,
            replicatedLogManager_->ProgressVectorValue->LastProgressVectorEntry.Lsn);
    }

    primaryStatus_ = PrimaryElectionStatus::Elected;
    roleContextDrainState_->ChangeRole(FABRIC_REPLICA_ROLE_PRIMARY);

    primaryTransactionsAbortingTask_.Put(CompletionTask::CreateAwaitableCompletionSource<void>(LOGGINGREPLICATOR_TAG, GetThisAllocator()).RawPtr());

    CancellationTokenSource::SPtr cts;
    NTSTATUS status = CancellationTokenSource::Create(
        GetThisAllocator(), 
        LOGGINGREPLICATOR_TAG, 
        cts);

    THROW_ON_FAILURE(status);

    primaryTransactionsAbortingCts_.Put(Ktl::Move(cts));

    AbortPendingTransactionsOrAddBarrierOnBecomingPrimary(
        abortingTransactions,
        primaryTransactionsAbortingTask_,
        primaryTransactionsAbortingCts_);

    // TODO: Add StartTelemetryTaskAsync here.
    backupManager_->EnableBackup();

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::CancelTheAbortingOfTransactionsIfNecessaryAsync() noexcept
{
    AwaitableCompletionSource<void>::SPtr completionSourceSPtr = primaryTransactionsAbortingTask_.Get();
    CancellationTokenSource::SPtr cancellationTokenSPtr = primaryTransactionsAbortingCts_.Get();
    
    if (cancellationTokenSPtr != nullptr)
    {
        ASSERT_IFNOT(
            completionSourceSPtr != nullptr,
            "{0}: CancelTheAbortingOfTransactionsIfNecessaryAsync Completion AwaitableCompletionSource must not be null",
            TraceId);

        cancellationTokenSPtr->Cancel();

        co_await completionSourceSPtr->GetAwaitable();

        primaryTransactionsAbortingTask_.Put(nullptr);
        primaryTransactionsAbortingCts_.Put(nullptr);
    }
    else
    {
        ASSERT_IFNOT(
            completionSourceSPtr == nullptr,
            "{0}: CancelTheAbortingOfTransactionsIfNecessaryAsync: primaryTransactionsAbortingTask_ must be null if the cts is nullptr",
            TraceId);
    }

    co_return STATUS_SUCCESS;
}

Task LoggingReplicatorImpl::AbortPendingTransactionsOrAddBarrierOnBecomingPrimary(
    __in KSharedArray<BeginTransactionOperationLogRecord::SPtr>::SPtr abortingTransactions,
    __in ThreadSafeSPtrCache<AwaitableCompletionSource<void>> & completingSource,
    __in ThreadSafeSPtrCache<CancellationTokenSource> & cts) noexcept
{
    KCoShared$ApiEntry();

    NTSTATUS status = STATUS_SUCCESS;
    KSharedArray<BeginTransactionOperationLogRecord::SPtr>::SPtr abortingTransactionsSPtr = abortingTransactions;
    AwaitableCompletionSource<void>::SPtr completionSourceSPtr = completingSource.Get();
    CancellationTokenSource::SPtr cancellationTokenSPtr = cts.Get();
    
    // Ensure that when the method exits, we signal the completing source
    KFinally([&]
    {
        completionSourceSPtr->Set();
    });
    
    ASSERT_IFNOT(
        abortingTransactionsSPtr != nullptr,
        "{0}: Aborting transactions must not be null",
        TraceId);

    ULONG abortDelay = 100;
    ULONG maxAbortDelay = 5000;

    if (abortingTransactionsSPtr->Count() > 0)
    {
        status = co_await KTimer::StartTimerAsync(GetThisAllocator(), LOGGINGREPLICATOR_TAG, abortDelay, nullptr);
        THROW_ON_FAILURE(status);

        ULONG index = 0;

        while (index < abortingTransactionsSPtr->Count())
        {
            BeginTransactionOperationLogRecord::SPtr beginRecord = (*abortingTransactionsSPtr)[index];
            EndTransactionLogRecord::SPtr endRecord = nullptr;
            
            status = co_await transactionManager_->EndTransactionAsync(
                beginRecord->BaseTransaction,
                false,
                false,
                endRecord);

            if (NT_SUCCESS(status))
            {

                ASSERT_IFNOT(
                    endRecord->IsEnlistedTransaction,
                    "{0}: endRecord->IsEnlistedTransaction must be true while aborting pending transactions on primary cr for tx {1}",
                    TraceId,
                    endRecord->BaseTransaction.TransactionId);

                EventSource::Events->BecomePrimaryAbortTransaction(
                    TracePartitionId,
                    ReplicaId,
                    endRecord->RecordType,
                    endRecord->Lsn,
                    endRecord->Psn,
                    endRecord->RecordPosition,
                    endRecord->BaseTransaction.TransactionId);

                // transaction aborted successfully.
                index++;

                // go to the next transaction
                continue;
            }

            // If still primary, back off and retry
            if (roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_PRIMARY &&
                !roleContextDrainState_->IsClosing &&
                !cancellationTokenSPtr->IsCancellationRequested)
            {
                abortDelay *= 2;

                if (abortDelay > maxAbortDelay)
                {
                    abortDelay = maxAbortDelay;
                }

                status = co_await KTimer::StartTimerAsync(GetThisAllocator(), LOGGINGREPLICATOR_TAG, abortDelay, nullptr);
                THROW_ON_FAILURE(status);
            }
            else
            {
                // If this replica is no longer primary or is closing or we are cancelled, stop retrying
                for (ULONG i = index; i < abortingTransactionsSPtr->Count(); i++)
                {

                    EventSource::Events->BecomePrimaryAbortTransactionStop(
                        TracePartitionId,
                        ReplicaId,
                        (*abortingTransactionsSPtr)[i]->RecordType,
                        (*abortingTransactionsSPtr)[i]->Lsn,
                        (*abortingTransactionsSPtr)[i]->Psn,
                        (*abortingTransactionsSPtr)[i]->RecordPosition,
                        (*abortingTransactionsSPtr)[i]->BaseTransaction.TransactionId);
                }
                
                // break out of while loop
                break;
            }
        }
    }
    else
    {
        // Try adding a barrier
        while (true)
        {
            status = co_await ReplicateBarrierIfPrimaryStatusIsBelowEstablishedAsync();

            if (NT_SUCCESS(status))
            {
                break;
            }

            // If still primary, back off and retry
            if (roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_PRIMARY &&
                !roleContextDrainState_->IsClosing &&
                !cancellationTokenSPtr->IsCancellationRequested)
            {
                abortDelay *= 2;
                if (abortDelay > maxAbortDelay)
                {
                    abortDelay = maxAbortDelay;
                }

                status = co_await KTimer::StartTimerAsync(GetThisAllocator(), LOGGINGREPLICATOR_TAG, abortDelay, nullptr);
                THROW_ON_FAILURE(status);
            }
            else
            {
                // break out of while loop
                break;
            }
        }
    }

    co_return;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::DeleteLogAsync() noexcept
{
    EventSource::Events->DeleteLog(
        TracePartitionId,
        ReplicaId);
    
    NTSTATUS status = co_await logManager_->DeleteLogAsync();

    co_return status;
}

void LoggingReplicatorImpl::Dispose()
{
    if (!isDisposed_)
    {
        stateProvider_->Dispose(); //this is needed to break cycle between stateprovider and statereplicator
        recoveryManager_->DisposeRecoveryReadStreamIfNeeded();

        if (logManager_ != nullptr)
        {
            logManager_->Dispose();
        }

        isDisposed_ = true;
    }
}

void LoggingReplicatorImpl::ProcessFlushedRecordsCallback(__in LoggedRecords const & loggedRecords) noexcept
{
    NTSTATUS status = loggedRecords.LogError;
    std::vector<FlushedRecordInfo> flushInfoVector;
    flushInfoVector.reserve(transactionalReplicatorConfig_->FlushedRecordsTraceVectorSize);

    for (ULONG i = 0; i < loggedRecords.Count; i++)
    {
        flushInfoVector.push_back(
            FlushedRecordInfo(
                loggedRecords[i]->RecordType,
                loggedRecords[i]->Lsn,
                loggedRecords[i]->Psn,
                loggedRecords[i]->RecordPosition));

        if (static_cast<int64>(flushInfoVector.size()) == transactionalReplicatorConfig_->FlushedRecordsTraceVectorSize)
        {
            // Trace records and reset vector
            EventSource::Events->FlushedRecordsCallback(
                TracePartitionId,
                ReplicaId,
                status,
                flushInfoVector);

            flushInfoVector.clear();
        }

        if (status != STATUS_SUCCESS)
        {
            operationProcessor_->ProcessLogError(
                L"ProcessFlushedRecordsCallback",
                *loggedRecords[i],
                status);
        }

        LogicalLogRecord * logicalRecord = loggedRecords[i]->AsLogicalLogRecord();

        if (logicalRecord != nullptr)
        {
            logicalRecord->ReleaseSerializedLogicalDataBuffers();
        }

        loggedRecords[i]->CompletedFlush(status);
    }

    // Trace any remaining flushed record information
    if (flushInfoVector.size() > 0)
    {
        EventSource::Events->FlushedRecordsCallback(
            TracePartitionId,
            ReplicaId,
            status,
            flushInfoVector);

        flushInfoVector.clear();
    }

    WakeupWaitingStreams(*loggedRecords[loggedRecords.Count - 1], status);
    logRecordsDispatcher_->DispatchLoggedRecords(loggedRecords);
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::PerformUpdateEpochAsync(
    __in Epoch const & epoch,
    __in LONG64 previousEpochLastLsn) noexcept
{
    Epoch localEpoch = epoch;

    EventSource::Events->UpdateEpoch(
        TracePartitionId,
        ReplicaId,
        localEpoch.DataLossVersion,
        localEpoch.ConfigurationVersion,
        previousEpochLastLsn,
        static_cast<int>(roleContextDrainState_->ReplicaRole),
        roleContextDrainState_->DrainStream);

    // This will ensure any pending replicates that haven't been logged will first be logged before
    // inserting the updateepoch log record
    if (roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_PRIMARY)
    {
        co_await replicatedLogManager_->AwaitLsnOrderingTaskOnPrimaryAsync();
    }

    UpdateEpochLogRecord::SPtr record = UpdateEpochLogRecord::Create(
        localEpoch,
        ReplicaId,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());

    ProgressVectorEntry lastVector = replicatedLogManager_->ProgressVectorValue->LastProgressVectorEntry;

    ASSERT_IFNOT(
        (lastVector.CurrentEpoch <= localEpoch) &&
        (lastVector.Lsn <= previousEpochLastLsn),
        "{0}:PerformUpdateEpochAsync Incoming epoch ({1},{2:x}) is less than the last known epoch ({3},{4:x})",
        TraceId,
        localEpoch.DataLossVersion,
        localEpoch.ConfigurationVersion,
        lastVector.CurrentEpoch.DataLossVersion,
        lastVector.CurrentEpoch.ConfigurationVersion);

    if (localEpoch == lastVector.CurrentEpoch)
    {
        co_return STATUS_SUCCESS;
    }

    // During copy, it is possible we end up copying MORE than what V1 replicator asks to copy.( Copied upto LSN 100 instead of 90)
    // V1 replicator does not know this. As a result the REPL queue is initialized to 90.
    // Now, if the primary goes down and this replica gets picked as Primary (I->P) transition, it is possible we get
    // UE call with e,90 which we cannot handle because we have already loggd upto 100.
    if (replicatedLogManager_->CurrentLogTailLsn != previousEpochLastLsn)
    {
        ASSERT_IFNOT(
            roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
            "{0}:PerformUpdateEpochAsync Got UE with {1}:{2:x} for lsn {3} while the tail is {4} and role is not idle (5)",
            TraceId,
            localEpoch.DataLossVersion,
            localEpoch.ConfigurationVersion,
            previousEpochLastLsn,
            replicatedLogManager_->CurrentLogTailLsn,
            roleContextDrainState_->ReplicaRole);

        co_return SF_STATUS_INVALID_OPERATION;
    }

    record->Lsn = previousEpochLastLsn;
    replicatedLogManager_->UpdateEpochRecord(*record);

    // Epoch update is a barrier
    co_await logManager_->FlushAsync(L"PerformUpdateEpochAsync");

    ASSERT_IFNOT(
        replicatedLogManager_->CurrentLogTailLsn == previousEpochLastLsn,
        "{0}:PerformUpdateEpochAsyncPost: replicatedLogManager_->CurrentLogTailLsn {1} == previousEpochLastLsn {2}",
        TraceId,
        replicatedLogManager_->CurrentLogTailLsn,
        previousEpochLastLsn);

    co_await record->AwaitProcessing();

    co_return STATUS_SUCCESS;
}

NTSTATUS LoggingReplicatorImpl::ErrorIfNoWriteStatus() const noexcept
{
    if (roleContextDrainState_->IsClosing == true ||
        roleContextDrainState_->ReplicaRole != FABRIC_REPLICA_ROLE_PRIMARY ||
        primaryStatus_ < PrimaryElectionStatus::Retained)
    {
        return SF_STATUS_NOT_PRIMARY;
    }

    return STATUS_SUCCESS;
}

Awaitable<FABRIC_REPLICA_ROLE> LoggingReplicatorImpl::PrepareForReplicaRoleTransitionAsync(__in FABRIC_REPLICA_ROLE newRole) noexcept
{
    FABRIC_REPLICA_ROLE currentRole = roleContextDrainState_->ReplicaRole;

    if (currentRole == FABRIC_REPLICA_ROLE_IDLE_SECONDARY &&
        newRole == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
    {
        // If copy was CopyStateNone, this is already completed.
        // Ensures that the copystream is drained before allowing the change role.

        co_await secondaryDrainManager_->WaitForCopyDrainToCompleteAsync();
        roleContextDrainState_->ChangeRole(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
    }
    else if (
        currentRole == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY &&
        newRole == FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
    {
        ASSERT_IFNOT(
            false,
            "LoggingReplicatorImpl::PrepareForReplicaRoleTransitionAsync with currentRole:{0}, newRole:{1} should never be called",
            currentRole,
            newRole);
    }
    else
    {
        co_await secondaryDrainManager_->WaitForCopyAndReplicationDrainToCompleteAsync();

        if (currentRole == FABRIC_REPLICA_ROLE_PRIMARY)
        {
            co_await replicatedLogManager_->AwaitLsnOrderingTaskOnPrimaryAsync();
            co_await backupManager_->DisableBackupAndDrainAsync();
            
            // Drain the aborting of pending transactions if any
            co_await CancelTheAbortingOfTransactionsIfNecessaryAsync();
        }

        if (newRole == FABRIC_REPLICA_ROLE_UNKNOWN || newRole == FABRIC_REPLICA_ROLE_NONE)
        {
            roleContextDrainState_->OnClosing();

            // Do not log additional record if 'RemovingState' is already logged earlier
            if (!recoveryManager_->IsRemovingStateAfterOpen)
            {
                InformationEvent::Enum informationEvent = (newRole == FABRIC_REPLICA_ROLE_NONE) ?
                    InformationEvent::Enum::RemovingState :
                    InformationEvent::Enum::Closed;

                co_await replicatedLogManager_->FlushInformationRecordAsync(
                    informationEvent,
                    true,
                    L"Closing/Removing state in PrepareForReplicaRoleTransitionAsync");
            }
        }
        else if (currentRole == FABRIC_REPLICA_ROLE_PRIMARY)
        {
            co_await replicatedLogManager_->FlushInformationRecordAsync(
                InformationEvent::Enum::PrimarySwap,
                false,
                L"Swapping out of primary");
        }

        // Never wait for physical log records processing as pending checkpoint has not been cancelled yet
        co_await QuiesceReplicaActivityAsync();

        roleContextDrainState_->ChangeRole(newRole);

        if (currentRole == FABRIC_REPLICA_ROLE_PRIMARY)
        {
            PrepareToTransitionOutOfPrimary(newRole == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        }
    }

    ASSERT_IFNOT(
        newRole == roleContextDrainState_->ReplicaRole,
        "New role: {0} does not match role context drain state role: {1}", 
        newRole, roleContextDrainState_->ReplicaRole);

    co_return currentRole;
}

Awaitable<void> LoggingReplicatorImpl::QuiesceReplicaActivityAsync() noexcept
{
    if (!LogRecord::IsInvalid(replicatedLogManager_->LastInformationRecord.RawPtr()))
    {
        InformationLogRecord::SPtr quiescedRecord = replicatedLogManager_->LastInformationRecord;
        co_await logManager_->FlushAsync(L"QuiesceReplicaActivity");

        EventSource::Events->QuiesceActivity(
            TracePartitionId,
            ReplicaId,
            quiescedRecord->RecordType,
            quiescedRecord->Lsn,
            quiescedRecord->Psn,
            quiescedRecord->RecordPosition,
            false);

        co_await quiescedRecord->AwaitProcessing();

        // NOTE - It is possible for the last information record to be overwritten to "ReplicationFinished" since the same drain task is used during copy and replication pump
        ASSERT_IFNOT(
            quiescedRecord == replicatedLogManager_->LastInformationRecord,
            "{0}: QuesicedRecord type is {1} lsn: {2} not equal to last information record {3} lsn: {4}",
            TraceId,
            quiescedRecord->RecordType,
            quiescedRecord->Lsn,
            replicatedLogManager_->LastInformationRecord->RecordType,
            replicatedLogManager_->LastInformationRecord->Lsn);

        EventSource::Events->QuiesceActivity(
            TracePartitionId,
            ReplicaId,
            quiescedRecord->RecordType,
            quiescedRecord->Lsn,
            quiescedRecord->Psn,
            quiescedRecord->RecordPosition,
            true);
    }

    // This method SHOULD ONLY wait for logical processing and not physical as pending 
    // checkpoints may not be cancelled yet
    co_await operationProcessor_->WaitForLogicalRecordsProcessingAsync();

    co_return;
}

// Summary: Called by Reliability through V1 Replicator when a suspected data-loss is encountered.
//
// Algorithm:
// 1. Enable restores
// 2. Call user's datalossHandler
// 3. Disable restores.
Awaitable<NTSTATUS> LoggingReplicatorImpl::OnDataLossAsync(__out bool & result) noexcept
{
    result = false;

    KShared$ApiEntry();

    backupManager_->EnableRestore();
    KFinally([&] {backupManager_->DisableRestore(); });

    NTSTATUS status = STATUS_SUCCESS;

    try
    {
        result = co_await dataLossHandler_->DataLossAsync(CancellationToken::None);
    }
    catch (Exception const & exception)
    {
        EventSource::Events->OnDataLossAsync(
            TracePartitionId,
            ReplicaId,
            L"OnDataLossAsync: DataLossHandler DataLossAsync call throw exception",
            exception.GetStatus());

        status = exception.GetStatus();
    }

    co_return status;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::UpdateEpochAsync(
    __in Epoch const & epoch,
    __in LONG64 lastLsnInPreviousEpoch) noexcept
{
    NTSTATUS status = co_await PerformUpdateEpochAsync(epoch, lastLsnInPreviousEpoch);

    co_return status;
}

 NTSTATUS LoggingReplicatorImpl::GetCopyContext(__out IOperationDataStream::SPtr & result) noexcept
{
    EventSource::Events->GetCopyContext(
        TracePartitionId,
        ReplicaId);
    
    CopyContext::SPtr copyContext = CopyContext::Create(
        *PartitionedReplicaIdentifier,
        ReplicaId,
        *replicatedLogManager_->ProgressVectorValue,
        *replicatedLogManager_->CurrentLogHeadRecord,
        replicatedLogManager_->CurrentLogTailLsn,
        recoveryManager_->LastRecoveredAtomicRedoOperationLsn,
        GetThisAllocator());
    
    result = IOperationDataStream::SPtr(copyContext.RawPtr());

    return STATUS_SUCCESS;
}

 NTSTATUS LoggingReplicatorImpl::GetCopyState(
    __in LONG64 uptoLsn,
    __in OperationDataStream & copyContext,
    __out IOperationDataStream::SPtr & result) noexcept
{
    EventSource::Events->GetCopyState(
        TracePartitionId,
        ReplicaId,
        uptoLsn);

    CopyStream::WaitForLogFlushUpToLsnCallback callback(this, &LoggingReplicatorImpl::WaitForLogFlushUptoLsn);
    
    CopyStream::SPtr stream = CopyStream::Create(
        *PartitionedReplicaIdentifier,
        *replicatedLogManager_,
        *stateManager_,
        *checkpointManager_,
        callback,
        ReplicaId,
        uptoLsn,
        copyContext,
        *copyStageBuffers_,
        transactionalReplicatorConfig_,
        hasPersistedState_,
        GetThisAllocator());
    
    result = IOperationDataStream::SPtr(stream.RawPtr());

    return STATUS_SUCCESS;
}

void LoggingReplicatorImpl::Test_SetTestHookContext(TestHooks::TestHookContext const & testHookContext)
{
    if (secondaryDrainManager_)
    {
        secondaryDrainManager_->Test_SetTestHookContext(testHookContext);
    }
}

NTSTATUS LoggingReplicatorImpl::GetSafeLsnToRemoveStateProvider(__out LONG64 & lsn) noexcept
{
    lsn = Constants::ZeroLsn;

    LogRecord::SPtr earliestRecord = replicatedLogManager_->GetEarliestRecord();
    lsn =
        (earliestRecord == nullptr) ?
        0 :
        earliestRecord->Lsn;

    LONG64 earliestFullCopyLogStartingLsn = logManager_->GetEarliestFullCopyStartingLsn();

    // If there was a full copy, return the LSN of the copy if it is smaller than the earliest record lsn
    if (earliestFullCopyLogStartingLsn < lsn)
    {
        lsn = earliestFullCopyLogStartingLsn;
    }

    return STATUS_SUCCESS;
}

void LoggingReplicatorImpl::ProcessedLogicalRecord(__in LogicalLogRecord & logicalRecord) noexcept
{
    operationProcessor_->ProcessedLogicalRecord(logicalRecord);
}

void LoggingReplicatorImpl::ProcessedPhysicalRecord(__in PhysicalLogRecord & physicalRecord) noexcept
{
    operationProcessor_->ProcessedPhysicalRecord(physicalRecord);
}

void LoggingReplicatorImpl::PrepareToTransitionOutOfPrimary(__in bool isSwapping) noexcept
{
    KArray<TransactionLogRecord::SPtr> pendingRecords(GetThisAllocator());
    transactionManager_->TransactionMapObject->GetPendingRecords(pendingRecords);

    for (ULONG i = 0; i < pendingRecords.Count(); i++)
    {
        TransactionLogRecord::SPtr record = pendingRecords[i];

        do
        {
            OperationLogRecord::SPtr operationRecord = dynamic_cast<OperationLogRecord *>(record.RawPtr());

            if (operationRecord != nullptr && !operationRecord->IsOperationContextNull)
            {
                OperationContext::CSPtr operationContext = operationRecord->ResetOperationContext();
                if (operationContext != nullptr)
                {
                    EventSource::Events->TransitionOutOfPrimaryUnlock(
                        TracePartitionId,
                        ReplicaId,
                        operationRecord->RecordType,
                        operationRecord->Lsn,
                        operationRecord->Psn,
                        operationRecord->RecordPosition,
                        operationRecord->BaseTransaction.TransactionId);

                    stateManager_->Unlock(*operationContext);
                }
            }

            record = record->ParentTransactionRecord;
        } while (record != nullptr);
    }

    primaryStatus_ = isSwapping ?
        PrimaryElectionStatus::SwappingOut :
        PrimaryElectionStatus::None;
}

Awaitable<NTSTATUS> LoggingReplicatorImpl::WaitForLogFlushUptoLsn(__in LONG64 uptoLsn) noexcept
{
    bool isWaiting = (uptoLsn > logManager_->CurrentLogTailRecord->Lsn);
    AwaitableCompletionSource<NTSTATUS>::SPtr tcs = nullptr;
    NTSTATUS result = STATUS_SUCCESS;

    if (isWaiting)
    {
        K_LOCK_BLOCK(flushWaitLock_)
        {
            isWaiting = (uptoLsn > logManager_->CurrentLogTailRecord->Lsn);

            if (isWaiting)
            {
                ULONG i;
                for (i = 0; i < waitingStreams_.Count(); i++)
                {
                    if (uptoLsn < waitingStreams_[i].Lsn)
                    {
                        break;
                    }
                }

                tcs = CompletionTask::CreateAwaitableCompletionSource<NTSTATUS>(LOGGINGREPLICATOR_TAG, GetThisAllocator());
                WaitingFlushStreamsContext context;
                context.Tcs = tcs;
                context.Lsn = uptoLsn;

                waitingStreams_.InsertAt(i, context);
            }
        }

        if (isWaiting)
        {
            co_await logManager_->FlushAsync(L"WaitForLogFlushUptoLsn");
            result = co_await tcs->GetAwaitable();
        }
    }

    co_return result;
}

void LoggingReplicatorImpl::WakeupWaitingStreams(
    __in LogRecord & mostRecentFlushedRecord,
    __in NTSTATUS flushError) noexcept
{
    LONG64 mostRecentFlushedLsn = mostRecentFlushedRecord.Lsn;
    KSharedArray<WaitingFlushStreamsContext>::SPtr streams = nullptr;

    K_LOCK_BLOCK(flushWaitLock_)
    {
        ULONG i = 0;
        for (i = 0; i < waitingStreams_.Count(); i++)
        {
            WaitingFlushStreamsContext stream = waitingStreams_[i];

            if (stream.Lsn <= mostRecentFlushedLsn)
            {
                if (streams == nullptr)
                {
                    streams = _new(LOGGINGREPLICATOR_TAG, GetThisAllocator())KSharedArray<WaitingFlushStreamsContext>();
                    ASSERT_IFNOT(
                        streams,
                        "{0}: failed to allocate streams in WakeupWaitingStreams",
                        TraceId);
                }

                NTSTATUS status = streams->Append(stream);
                ASSERT_IFNOT(
                    NT_SUCCESS(status),
                    "{0}: failed to append to stream array",
                    TraceId);

                continue;
            }

            break;
        }
            
        if (i > 0)
        {
            BOOLEAN removedResult = waitingStreams_.RemoveRange(0, i);
            ASSERT_IFNOT(removedResult == TRUE, "Could not remove from lsn ordering queue");
        }
    }

    if (streams != nullptr)
    {
        for (ULONG i = 0; i < streams->Count(); i++)
        {
            WaitingFlushStreamsContext stream = (*streams)[i];
            stream.Tcs->SetResult(flushError);
        }
    }

    return;
}

void LoggingReplicatorImpl::Reuse()
{
    isDisposed_ = false;

    recoveredOrCopiedCheckpointState_->Update(Constants::InvalidLsn);
    roleContextDrainState_->Reuse();
    recoveryManager_->Reuse();
    operationProcessor_->Reuse();
    waitingStreams_.Clear();
    checkpointManager_->Reuse();
    replicatedLogManager_->Reuse();
    primaryStatus_ = PrimaryElectionStatus::None;
    primaryTransactionsAbortingCts_.Put(nullptr);
    primaryTransactionsAbortingTask_.Put(nullptr);
    versionManager_->Reuse();
    secondaryDrainManager_->Reuse();
}

void LoggingReplicatorImpl::CreateAllManagers()
{
    EventSource::Events->CreateAllManagers(
        TracePartitionId,
        ReplicaId);

    TransactionMap::SPtr transactionMap = TransactionMap::Create(
        *PartitionedReplicaIdentifier,
        GetThisAllocator());

    checkpointManager_ = CheckpointManager::Create(
        *PartitionedReplicaIdentifier,
        *logTruncationManager_,
        *recoveredOrCopiedCheckpointState_,
        *replicatedLogManager_,
        *transactionMap,
        *stateManager_,
        *backupManager_,
        transactionalReplicatorConfig_,
        *invalidLogRecords_,
        perfCounters_,
        healthClient_,
        GetThisAllocator());

    CreateOperationProcessor();

    if (transactionalReplicatorConfig_->DispatchingMode.compare(Constants::SerialDispatchingMode) == 0)
    {
        logRecordsDispatcher_ = SerialLogRecordsDispatcher::Create(
            *PartitionedReplicaIdentifier,
            *operationProcessor_,
            transactionalReplicatorConfig_,
            GetThisAllocator());
    }
    else
    {
        logRecordsDispatcher_ = ParallelLogRecordsDispatcher::Create(
            *PartitionedReplicaIdentifier,
            *operationProcessor_,
            transactionalReplicatorConfig_,
            GetThisAllocator());
    }

    transactionManager_ = TransactionManager::Create(
        *PartitionedReplicaIdentifier,
        *recoveredOrCopiedCheckpointState_,
        *transactionMap,
        *checkpointManager_,
        *replicatedLogManager_,
        *operationProcessor_,
        *invalidLogRecords_,
        transactionalReplicatorConfig_,
        perfCounters_,
        GetThisAllocator());

    secondaryDrainManager_ = SecondaryDrainManager::Create(
        *PartitionedReplicaIdentifier,
        *recoveredOrCopiedCheckpointState_,
        *roleContextDrainState_,
        *operationProcessor_,
        *iStateReplicator_,
        *backupManager_,
        *checkpointManager_,
        *transactionManager_,
        *replicatedLogManager_,
        *stateManager_,
        *recoveryManager_,
        transactionalReplicatorConfig_,
        *invalidLogRecords_,
        GetThisAllocator());

    // Update the dependencies of several components.
    ReplicatedLogManager::AppendCheckpointCallback callback(checkpointManager_.RawPtr(), &CheckpointManager::CheckpointIfNecessary);
    replicatedLogManager_->SetCheckpointCallback(callback);

    logFlushCallbackManager_->FlushCallbackProcessor = *this;
    checkpointManager_->CompletedRecordsProcessor = *this;
    backupManager_->Initialize(*this, *checkpointManager_, *operationProcessor_);
    versionManager_->Initialize(*this);
}

void LoggingReplicatorImpl::CreateOperationProcessor()
{
    K_LOCK_BLOCK(changeHandlerLock_)
    {
        // Create the operationProcessor: it might be the first time to create operationProcessor or recreate, in the 
        // recreate case, we need to save the handler and re-register to the operationProcessor.
        // Possible case:
        //      OperationProcessor == nullptr && Cache == nullptr: create OperationProcessor
        //      OperationProcessor == nullptr && Cache != nullptr: create OperationProcessor, register the handler and reset cache to nullptr
        //      OperationProcessor != nullptr && handlerRegistred && Cache == nullptr: save the handler, recreate the operationProcessor and re-register the handler
        //      OperationProcessor != nullptr && !handlerRegistred && Cache == nullptr: create OperationProcessor
        // Invariant:
        //      OperationProcessor != nullptr && handlerRegistred && Cache != nullptr: If OperationProcessor not nullptr, cache must be clean
        //      OperationProcessor != nullptr && !handlerRegistred && Cache != nullptr: If OperationProcessor not nullptr, cache must be clean
        ITransactionChangeHandler::SPtr eventHandler = changeHandlerCache_;

        ASSERT_IF(
            operationProcessor_ != nullptr && eventHandler != nullptr,
            "{0}: CreateAllManagers: If OperationProcessor not nullptr, cache must be clean",
            TraceId);

        if (operationProcessor_ != nullptr)
        {
            changeHandlerCache_ = operationProcessor_->ChangeHandler;
        }

        operationProcessor_ = OperationProcessor::Create(
            *PartitionedReplicaIdentifier,
            *recoveredOrCopiedCheckpointState_,
            *roleContextDrainState_,
            *versionManager_,
            *checkpointManager_,
            *stateManager_,
            *backupManager_,
            *invalidLogRecords_,
            transactionalReplicatorConfig_,
            iTransactionalReplicator_,
            GetThisAllocator());

        eventHandler = changeHandlerCache_;
        if (eventHandler != nullptr)
        {
            NTSTATUS status = operationProcessor_->RegisterTransactionChangeHandler(*eventHandler);
            ASSERT_IFNOT(
                NT_SUCCESS(status),
                "{0}: RegisterTransactionChangeHandler from cache failed with status {1}",
                TraceId,
                status);
            changeHandlerCache_ = nullptr;
        }
    }

    return;
}

// Function used to register the change handler to operation processor.
// Possible case:
//      OperationProcessor == nullptr && Cache == nullptr: save the handler to cache
//      OperationProcessor == nullptr && Cache != nullptr: replace the handler in the cache
//      OperationProcessor != nullptr && Cache == nullptr: register the handler to operation processor
// Invariant:
//      OperationProcessor != nullptr && Cache != nullptr: Creation of operation processor will set cache to nullptr
NTSTATUS LoggingReplicatorImpl::RegisterTransactionChangeHandler(
    __in ITransactionChangeHandler & transactionChangeHandler) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    K_LOCK_BLOCK(changeHandlerLock_)
    {
        ITransactionChangeHandler::SPtr eventHandler = changeHandlerCache_;

        ASSERT_IF(
            operationProcessor_ != nullptr && eventHandler != nullptr,
            "{0}: RegisterTransactionChangeHandler: Creation of operation processor will set cache to nullptr",
            TraceId);

        if (operationProcessor_ != nullptr)
        {
            status = operationProcessor_->RegisterTransactionChangeHandler(transactionChangeHandler);
            return status;
        }
        
        changeHandlerCache_ = &transactionChangeHandler;
    }

    return STATUS_SUCCESS;
}

// Function used to un-register the change handler from operation processor.
// Possible case:
//      OperationProcessor == nullptr && Cache != nullptr: reset the cache to nullptr
//      OperationProcessor == nullptr && Cache == nullptr: reset the cache to nullptr
//      OperationProcessor != nullptr && Cache == nullptr: un-register the handler from operation processor
// Invariant:
//      OperationProcessor != nullptr && Cache != nullptr: only put handler to cache if OperationProcessor is nullptr
NTSTATUS LoggingReplicatorImpl::UnRegisterTransactionChangeHandler() noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    K_LOCK_BLOCK(changeHandlerLock_)
    {
        ITransactionChangeHandler::SPtr eventHandler = changeHandlerCache_;

        ASSERT_IF(
            operationProcessor_ != nullptr && eventHandler != nullptr,
            "{0}: UnRegisterTransactionChangeHandler: Only put handler to cache if OperationProcessor is nullptr",
            TraceId);

        if (operationProcessor_ == nullptr)
        {
            changeHandlerCache_ = nullptr;
            status = STATUS_SUCCESS;
        }

        status = operationProcessor_->UnRegisterTransactionChangeHandler();
    }

    return status;
}

NTSTATUS LoggingReplicatorImpl::Test_RequestCheckpointAfterNextTransaction() noexcept
{
    logTruncationManager_->ForceCheckpoint();

    return STATUS_SUCCESS;
}

NTSTATUS LoggingReplicatorImpl::Test_GetPeriodicCheckpointAndTruncationTimestampTicks(
    __out LONG64 & lastPeriodicCheckpointTimeTicks,
    __out LONG64 & lastPeriodicTruncationTimeTicks) noexcept
{
    if (checkpointManager_ == nullptr)
    {
        return SF_STATUS_OBJECT_CLOSED;
    }

    if (replicatedLogManager_->LastCompletedBeginCheckpointRecord->PeriodicCheckpointTimeTicks < checkpointManager_->LastPeriodicCheckpointTime.Ticks)
    {
        lastPeriodicCheckpointTimeTicks = replicatedLogManager_->LastCompletedBeginCheckpointRecord->PeriodicCheckpointTimeTicks;
    }
    else
    {
        lastPeriodicCheckpointTimeTicks = checkpointManager_->LastPeriodicCheckpointTime.Ticks;
    }

    lastPeriodicTruncationTimeTicks = checkpointManager_->LastPeriodicTruncationTime.Ticks;

    return STATUS_SUCCESS;
}